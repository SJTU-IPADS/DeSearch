import json # for uncode json
import os

def check_json():
    path = "blocks/"
    files = os.listdir(path)
    operation_types = {}
    for f in files:
        try:
            f_o = open("blocks/" +f,"rb")
        except:
            print("open ",f," fail!")
            break
        j = f_o.readline()
        try:
            data = json.loads(j)
        except:
            print("load json fail!")
            break
        if(data["result"] and data["result"]["block"] and data["result"]["block"]["transactions"]):
            for tran in data["result"]["block"]["transactions"]:
                if(tran["operations"]):
                    for op in tran["operations"]:
                        if(op["type"] and op["type"] == "comment_operation"):
                            if(op["value"]):
                                if(op["value"]["title"]):
                                    print(op["value"]["title"])
                                if(op["value"]["body"]):
                                    print(op["value"]["body"])
                                    print("----------------------------------------------------------------")
    return operation_types


def main():
    types = check_json()
    print("-----------------")
    print(types)

if __name__ == '__main__':
    main()