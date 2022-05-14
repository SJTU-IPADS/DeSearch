import requests # for openbazaar API
import json # str => dict, list, etc.
import queue # for queue
import threading # multi-thread program
from multiprocessing import Process
import pickle # data save
import os # file ops
import logging # log

queue_lock = threading.Lock()
dict_lock = threading.Lock()

working_queue = queue.Queue()
all_dict = {}

def save_data(data,filename):
    logging.info("Start save " + filename)
    if(os.path.exists(filename)):
        try:
            os.rename(filename,filename + ".backup")
        except Exception as e:
            logging.error(filename + "backup failed, Reason: ",e)
    try:
        w_file = open(filename,"wb")
    except Exception as e:
        logging.error("open " + filename + "failed, Reason: ",e)
        logging.critical("exiting!")
        exit(-1)
    try:
        pickle.dump(data, w_file)
    except Exception as e:
        logging.error("save data to " + filename + " failed, Reason: ", e)
        exit(-1)
    logging.info("save " + filename + " finished")
    try:
        w_file.close()
    except Exception as e:
        logging.error("close " + filename + " failed, Reason: ",e)

    if(os.path.exists(filename + ".backup")):
        try:
            os.remove(filename + ".backup")
        except Exception as e:
            logging.error("remove " + filename + "'s backup failed, Reason: ",e)
        logging.info(filename + "'s backup removed")

def read_data(filename):
    try:
        r_file = open(filename,"rb")
    except Exception as e:
        logging.error("open " + filename + " failed, Reason: ",e)
        logging.critical("exiting!")
        exit(-1)
    return pickle.load(openbazaar_file)

def init_queue():
    logging.info("Start init queue")
    r = requests.get("http://localhost:4002/ob/peers")
    if(r.status_code != 200):
        logging.error("init queue fail!")
        logging.critical("exiting!")
        return
    init_peers = json.loads(r.content)
    global working_queue
    global all_dict
    for peer in init_peers:
        working_queue.put(peer)
        all_dict[peer] = 1
    logging.info("init queue succeed!")

def read_queue():
    global all_dict
    global working_queue
    queue_lock.acquire()
    if(working_queue.qsize != 0):
        try:
            res = working_queue.get()
        except Exception as e:
            logging.error("read queue failed, Reason: ",e)
            logging.critical("exiting!")
            exit(-1)
        finally:
            queue_lock.release()
        return res
    else:
        return None


def write_queue(peer):
    global working_queue
    global all_dict
    queue_lock.acquire()
    try:
        working_queue.put(peer)
    except Exception as e:
        logging.error("write queue failed, Reason: ",e)
        logging.critical("exiting!")
        exit(-1)
    finally:
        queue_lock.release()


def write_dict(peer_list):
    global all_dict
    global working_queue
    for peer in peer_list:
        dict_lock.acquire()
        try:
            if(peer not in all_dict):
                all_dict[peer] = 1
                write_queue(peer)
        except Exception as e:
            logging.error("write dictory failed, Reason: ",e)
            logging.critical("exiting!")
            exit(-1)
        finally:
            save_data(all_dict,"peers.pkl")
            dict_lock.release()

def crawler():
    global all_dict
    global working_queue
    peer = read_queue()
    while(peer):
        # new process to get peer's data
        process_get_peer_data = Process(target=get_peers_data, args = (peer,))
        process_get_peer_data.start()
        try:
            logging.info("crawling: http://localhost:4002/ob/closestpeers/" + peer)
            r = requests.get("http://localhost:4002/ob/closestpeers/" + peer,timeout=(300,300))
            if(r.status_code != 200):
                logging.error("request fail!")
                peer = read_queue()
                break;
            peers = json.loads(r.content)
            write_dict(peers)
        except Exception as e:
            logging.error("request fail!, Reason: ",e)
        peer = read_queue()

# use multiprocessing
def get_peers_data(peer):
    # make a dictory for peer
    try:
        os.mkdir(peer)
    except Exception as e:
        logging.error("mkdir " + peer + " failed")
        logging.critical("exiting")
        exit(-1)

    try:
        r_profile = requests.get("http://localhost:4002/ob/profile/" + peer,timeout=(300,300))
        if(r_profile.status_code != 200):
            logging.error("get profile fail!")
        else:
            try:
                save_data(json.loads(r_profile.content),peer + "/profile.pkl")
            except Exception as e:
                logging.error("save " + peer + "/profile.pkl failed!, Reason: ",e)
    except Exception as e:
        logging.error("requests http://localhost:4002/ob/profile/" + peer + " failed, Reason: ",e)

    try:
        r_listings = requests.get("http://localhost:4002/ob/listings/" + peer,timeout=(300,300))
        if(r_listings.status_code != 200):
            logging.error("get listings fail!")
        else:
            try:
                save_data(json.loads(r_listings.content),peer + "/listings.pkl")
            except Exception as e:
                logging.error("save " + peer + "/listings.pkl failed!, Reason: ",e)
            try:
                os.mkdir(peer + "/listings")
            except Exception as e:
                logging.error("mkdir " + peer + "/listings failed")
                logging.critical("exiting")
                exit(-1)
                for li in json.loads(r_listings.content):
                    try:
                        r_detail = requests.get("http://localhost:4002/ob/listing/" + peer + "/" + li['hash'],timeout=(300,300))
                        if(r_listings.status_code != 200):
                            logging.error("get listing " + peer + "/" + li['hash'] + " fail!")
                        else:
                            try:
                                save_data(json.loads(r_detail.content),peer + "/listings/listing-" + li['hash'] + ".pkl")
                            except Exception as e:
                                logging.error("save " + peer + "/listings/listing-" + li['hash'] + ".pkl failed!, Reason: ",e)
                    except Exception as e:
                        logging.error(" requests http://localhost:4002/ob/listing/" + peer + "/" + li['hash'] + " failed, Reason: " + e)
    except Exception as e:
        logging.error("request http://localhost:4002/ob/listings/" + peer + " failed, Reason: ",e)


def main():
    logging.basicConfig(filename='openbazaar_crawler.log',level=logging.INFO)
    logging.info("Start crawling")
    init_queue()

    t_list = []
    for i in range(10):
        t = threading.Thread(target = crawler)
        t.start()
        t_list.append(t)

    for t in t_list:
        t.join()

    # save dict
    save_data(all_dict)
    logging.info("-----------------Finished!!!------------------------")


if __name__ == '__main__':
    main()