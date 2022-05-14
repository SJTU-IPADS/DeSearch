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

#ifndef _sphinxrt_
#define _sphinxrt_

#include "sphinx.h"
#include "sphinxutils.h"
#include "sphinxstem.h"

struct CSphReconfigureSettings;
struct CSphReconfigureSetup;
class ISphRtAccum;


/// RAM based updateable backend interface
class ISphRtIndex : public CSphIndex
{
public:
	explicit ISphRtIndex ( const char * sIndexName, const char * sFileName ) : CSphIndex ( sIndexName, sFileName ) {}

	/// get internal schema (to use for Add calls)
	virtual const CSphSchema & GetInternalSchema () const { return m_tSchema; }

	/// insert/update document in current txn
	/// fails in case of two open txns to different indexes
	virtual bool AddDocument ( ISphTokenizer * pTokenizer, int iFields, const char ** ppFields, const CSphMatch & tDoc, bool bReplace, const CSphString & sTokenFilterOptions, const char ** ppStr, const CSphVector<DWORD> & dMvas, CSphString & sError, CSphString & sWarning, ISphRtAccum * pAccExt ) = 0;

	/// delete document in current txn
	/// fails in case of two open txns to different indexes
	virtual bool DeleteDocument ( const SphDocID_t * pDocs, int iDocs, CSphString & sError, ISphRtAccum * pAccExt ) = 0;

	/// commit pending changes
	virtual void Commit ( int * pDeleted, ISphRtAccum * pAccExt ) = 0;

	/// undo pending changes
	virtual void RollBack ( ISphRtAccum * pAccExt ) = 0;

	/// check and periodically flush RAM chunk to disk
	virtual void CheckRamFlush () = 0;

	/// forcibly flush RAM chunk to disk
	virtual void ForceRamFlush ( bool bPeriodic=false ) = 0;

	/// forcibly save RAM chunk as a new disk chunk
	virtual void ForceDiskChunk () = 0;

	/// attach a disk chunk to current index
	virtual bool AttachDiskIndex ( CSphIndex * pIndex, CSphString & sError ) = 0;

	/// truncate index (that is, kill all data)
	virtual bool Truncate ( CSphString & sError ) = 0;

	virtual void Optimize ( volatile bool * pForceTerminate, ThrottleState_t * pThrottle ) = 0;

	/// check settings vs current and return back tokenizer and dictionary in case of difference
	virtual bool IsSameSettings ( CSphReconfigureSettings & tSettings, CSphReconfigureSetup & tSetup, CSphString & sError ) const = 0;

	/// reconfigure index by using new tokenizer, dictionary and index settings
	/// current data got saved with current settings
	virtual void Reconfigure ( CSphReconfigureSetup & tSetup ) = 0;

	/// get disk chunk
	virtual CSphIndex * GetDiskChunk ( int iChunk ) = 0;
	
	virtual ISphRtAccum * CreateAccum ( CSphString & sError ) = 0;

	// instead of cloning for each AddDocument() call we could just call this method and improve batch inserts speed
	virtual ISphTokenizer * CloneIndexingTokenizer() const = 0;
};

/// initialize subsystem
class CSphConfigSection;
void sphRTInit ( const CSphConfigSection & hSearchd, bool bTestMode );
void sphRTConfigure ( const CSphConfigSection & hSearchd, bool bTestMode );
bool sphRTSchemaConfigure ( const CSphConfigSection & hIndex, CSphSchema * pSchema, CSphString * pError );
void sphRTSetTestMode ();

/// deinitialize subsystem
void sphRTDone ();

/// RT index factory
ISphRtIndex * sphCreateIndexRT ( const CSphSchema & tSchema, const char * sIndexName, int64_t iRamSize, const char * sPath, bool bKeywordDict );

typedef void ProgressCallbackSimple_t ();

class ISphRtAccum
{
protected:
	ISphRtIndex * m_pIndex;		///< my current owner in this thread
	ISphRtAccum () {} // can not create such thing outside of RT index
public:
	virtual ~ISphRtAccum () {}
	ISphRtIndex * GetIndex() const { return m_pIndex; }
};


//////////////////////////////////////////////////////////////////////////

enum ESphBinlogReplayFlags
{
	SPH_REPLAY_ACCEPT_DESC_TIMESTAMP = 1,
	SPH_REPLAY_IGNORE_OPEN_ERROR = 2
};

/// replay stored binlog
void sphReplayBinlog ( const SmallStringHash_T<CSphIndex*> & hIndexes, DWORD uReplayFlags, ProgressCallbackSimple_t * pfnProgressCallback=NULL );

#endif // _sphinxrt_

//
// $Id$
//
