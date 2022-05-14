//
// $Id$
//

//
// Copyright (c) 2001-2016, Andrew Aksyonoff
// Copyright (c) 2008-2016, Sphinx Technologies Inc
// All rights reserved
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License. You should have
// received a copy of the GPL license along with this program; if you
// did not, you can find it at http://www.gnu.org/
//

#ifndef _sphinxexpr_
#define _sphinxexpr_

#include "sphinxstd.h"

/// forward decls
class CSphMatch;
class ISphSchema;
class CSphSchema;
struct CSphString;
struct CSphColumnInfo;

/// known attribute types
enum ESphAttr
{
	// these types are full types
	// their typecodes are saved in the index schema, and thus,
	// TYPECODES MUST NOT CHANGE ONCE INTRODUCED
	SPH_ATTR_NONE		= 0,			///< not an attribute at all
	SPH_ATTR_INTEGER	= 1,			///< unsigned 32-bit integer
	SPH_ATTR_TIMESTAMP	= 2,			///< this attr is a timestamp
	// there was SPH_ATTR_ORDINAL=3 once
	SPH_ATTR_BOOL		= 4,			///< this attr is a boolean bit field
	SPH_ATTR_FLOAT		= 5,			///< floating point number (IEEE 32-bit)
	SPH_ATTR_BIGINT		= 6,			///< signed 64-bit integer
	SPH_ATTR_STRING		= 7,			///< string (binary; in-memory)
	// there was SPH_ATTR_WORDCOUNT=8 once
	SPH_ATTR_POLY2D		= 9,			///< vector of floats, 2D polygon (see POLY2D)
	SPH_ATTR_STRINGPTR	= 10,			///< string (binary, in-memory, stored as pointer to the zero-terminated string)
	SPH_ATTR_TOKENCOUNT	= 11,			///< field token count, 32-bit integer
	SPH_ATTR_JSON		= 12,			///< JSON subset; converted, packed, and stored as string

	SPH_ATTR_UINT32SET	= 0x40000001UL,	///< MVA, set of unsigned 32-bit integers
	SPH_ATTR_INT64SET	= 0x40000002UL,	///< MVA, set of signed 64-bit integers

	// these types are runtime only
	// used as intermediate types in the expression engine
	SPH_ATTR_MAPARG		= 1000,
	SPH_ATTR_FACTORS	= 1001,			///< packed search factors (binary, in-memory, pooled)
	SPH_ATTR_JSON_FIELD	= 1002,			///< points to particular field in JSON column subset
	SPH_ATTR_FACTORS_JSON	= 1003		///< packed search factors (binary, in-memory, pooled, provided to client json encoded)
};

/// column evaluation stage
enum ESphEvalStage
{
	SPH_EVAL_STATIC = 0,		///< static data, no real evaluation needed
	SPH_EVAL_OVERRIDE,			///< static but possibly overridden
	SPH_EVAL_PREFILTER,			///< expression needed for candidate matches filtering
	SPH_EVAL_PRESORT,			///< expression needed for final matches sorting
	SPH_EVAL_SORTER,			///< expression evaluated by sorter object
	SPH_EVAL_FINAL,				///< expression not (!) used in filters/sorting; can be postponed until final result set cooking
	SPH_EVAL_POSTLIMIT			///< expression needs to be postponed until we apply all the LIMIT clauses (say, too expensive)
};

/// expression tree wide commands
/// FIXME? maybe merge with ExtraData_e?
enum ESphExprCommand
{
	SPH_EXPR_SET_MVA_POOL,
	SPH_EXPR_SET_STRING_POOL,
	SPH_EXPR_SET_EXTRA_DATA,
	SPH_EXPR_GET_DEPENDENT_COLS, ///< used to determine proper evaluating stage
	SPH_EXPR_GET_UDF
};

/// expression evaluator
/// can always be evaluated in floats using Eval()
/// can sometimes be evaluated in integers using IntEval(), depending on type as returned from sphExprParse()
struct ISphExpr : public ISphRefcounted
{
public:
	/// evaluate this expression for that match
	virtual float Eval ( const CSphMatch & tMatch ) const = 0;

	/// evaluate this expression for that match, using int math
	virtual int IntEval ( const CSphMatch & tMatch ) const { assert ( 0 ); return (int) Eval ( tMatch ); }

	/// evaluate this expression for that match, using int64 math
	virtual int64_t Int64Eval ( const CSphMatch & tMatch ) const { assert ( 0 ); return (int64_t) Eval ( tMatch ); }

	/// Evaluate string attr.
	/// Note, that sometimes this method returns pointer to a static buffer
	/// and sometimes it allocates a new buffer, so aware of memory leaks.
	/// IsStringPtr() returns true if this method allocates a new buffer and false otherwise.
	virtual int StringEval ( const CSphMatch &, const BYTE ** ppStr ) const { *ppStr = NULL; return 0; }

	/// evaluate MVA attr
	virtual const DWORD * MvaEval ( const CSphMatch & ) const { assert ( 0 ); return NULL; }

	/// evaluate Packed factors
	virtual const DWORD * FactorEval ( const CSphMatch & ) const { assert ( 0 ); return NULL; }

	/// check for arglist subtype
	/// FIXME? replace with a single GetType() call?
	virtual bool IsArglist () const { return false; }

	/// check for stringptr subtype
	/// FIXME? replace with a single GetType() call?
	virtual bool IsStringPtr () const { return false; }

	/// get Nth arg of an arglist
	virtual ISphExpr * GetArg ( int ) const { return NULL; }

	/// get the number of args in an arglist
	virtual int GetNumArgs() const { return 0; }

	/// run a tree wide action (1st arg is an action, 2nd is its parameter)
	/// usually sets something into ISphExpr like string pool or gets something from it like dependent columns
	virtual void Command ( ESphExprCommand, void * ) {}

	/// check for const type
	virtual bool IsConst () const { return false; }

	/// get expression hash (for query cache)
	virtual uint64_t GetHash ( const ISphSchema & tSorterSchema, uint64_t uPrevHash, bool & bDisable ) = 0;
};

/// string expression traits
/// can never be evaluated in floats or integers, only StringEval() is allowed
struct ISphStringExpr : public ISphExpr
{
	virtual float Eval ( const CSphMatch & ) const { assert ( 0 && "one just does not simply evaluate a string as float" ); return 0; }
	virtual int IntEval ( const CSphMatch & ) const { assert ( 0 && "one just does not simply evaluate a string as int" ); return 0; }
	virtual int64_t Int64Eval ( const CSphMatch & ) const { assert ( 0 && "one just does not simply evaluate a string as bigint" ); return 0; }
};

/// hook to extend expressions
/// lets one to add her own identifier and function handlers
struct ISphExprHook
{
	virtual ~ISphExprHook () {}
	/// checks for an identifier known to the hook
	/// returns -1 on failure, a non-negative OID on success
	virtual int IsKnownIdent ( const char * sIdent ) = 0;

	/// checks for a valid function call
	/// returns -1 on failure, a non-negative OID on success (possibly adjusted)
	virtual int IsKnownFunc ( const char * sFunc ) = 0;

	/// create node by OID
	/// pEvalStage is an optional out-parameter
	/// hook may fill it, but that is *not* required
	virtual ISphExpr * CreateNode ( int iID, ISphExpr * pLeft, ESphEvalStage * pEvalStage, CSphString & sError ) = 0;

	/// get identifier return type by OID
	virtual ESphAttr GetIdentType ( int iID ) = 0;

	/// get function return type by OID and argument types vector
	/// must return SPH_ATTR_NONE and fill the message on failure
	virtual ESphAttr GetReturnType ( int iID, const CSphVector<ESphAttr> & dArgs, bool bAllConst, CSphString & sError ) = 0;

	/// recursive scope check
	virtual void CheckEnter ( int iID ) = 0;

	/// recursive scope check
	virtual void CheckExit ( int iID ) = 0;
};


/// named int/string variant
/// used for named expression function arguments block
/// ie. {..} part in, for example, BM25F(1.2, 0.8, {title=3}) call
struct CSphNamedVariant
{
	CSphString		m_sKey;		///< key
	CSphString		m_sValue;	///< value for strings, empty for ints
	int				m_iValue;	///< value for ints
};


/// a container used to pass maps of constants/variables around the evaluation tree
struct Expr_MapArg_c : public ISphExpr
{
	CSphVector<CSphNamedVariant> m_dValues;

	explicit Expr_MapArg_c ( CSphVector<CSphNamedVariant> & dValues )
	{
		m_dValues.SwapData ( dValues );
	}

	virtual float Eval ( const CSphMatch & ) const
	{
		assert ( 0 && "one just does not simply evaluate a const hash" );
		return 0.0f;
	}

	virtual uint64_t GetHash ( const ISphSchema &, uint64_t, bool & )
	{
		assert ( 0 && "calling GetHash from a const hash" );
		return 0;
	}
};


enum
{
	SPH_FACTOR_DISABLE		= 0,
	SPH_FACTOR_ENABLE		= 1,
	SPH_FACTOR_CALC_ATC		= 1 << 1,
	SPH_FACTOR_JSON_OUT		= 1 << 2
};


/// known collations
enum ESphCollation
{
	SPH_COLLATION_LIBC_CI,
	SPH_COLLATION_LIBC_CS,
	SPH_COLLATION_UTF8_GENERAL_CI,
	SPH_COLLATION_BINARY,

	SPH_COLLATION_DEFAULT = SPH_COLLATION_LIBC_CI
};

/// parses given expression, builds evaluator
/// returns NULL and fills sError on failure
/// returns pointer to evaluator on success
/// fills pAttrType with result type (for now, can be SPH_ATTR_SINT or SPH_ATTR_FLOAT)
/// fills pUsesWeight with a flag whether match relevance is referenced in expression AST
/// fills pEvalStage with a required (!) evaluation stage
class CSphQueryProfile;
ISphExpr * sphExprParse ( const char * sExpr, const ISphSchema & tSchema, ESphAttr * pAttrType, bool * pUsesWeight,
	CSphString & sError, CSphQueryProfile * pProfiler, ESphCollation eCollation=SPH_COLLATION_DEFAULT, ISphExprHook * pHook=NULL,
	bool * pZonespanlist=NULL, DWORD * pPackedFactorsFlags=NULL, ESphEvalStage * pEvalStage=NULL );

ISphExpr * sphJsonFieldConv ( ISphExpr * pExpr );

//////////////////////////////////////////////////////////////////////////

/// init tables used by our geodistance functions
void GeodistInit();

/// haversine sphere distance, radians
float GeodistSphereRad ( float lat1, float lon1, float lat2, float lon2 );

/// haversine sphere distance, degrees
float GeodistSphereDeg ( float lat1, float lon1, float lat2, float lon2 );

/// flat ellipsoid distance, degrees
float GeodistFlatDeg ( float fLat1, float fLon1, float fLat2, float fLon2 );

/// adaptive flat/haversine distance, degrees
float GeodistAdaptiveDeg ( float lat1, float lon1, float lat2, float lon2 );

/// adaptive flat/haversine distance, radians
float GeodistAdaptiveRad ( float lat1, float lon1, float lat2, float lon2 );

#endif // _sphinxexpr_

//
// $Id$
//
