package main
import (
    "fmt"
    "net/http"
    "io/ioutil"
    "bytes"
    "strconv"
    "sync"
    "encoding/json"
)


var block_count int64
var mutex sync.Mutex
var wait sync.WaitGroup


type steem_reward struct {
    amount int64
    nai string
    precision string
}

type steem_op_value struct { author string
    permlink string
    voter string
    weight int64
    reward_sbd steem_reward
    reward_steem steem_reward
    reward_vests steem_reward
}

type steem_operation struct {
    op_type string
    value steem_op_value
}

type steem_transaction struct {
    expiration string
    extensions []string
    operations []steem_operation
    ref_block_num int64
    ref_block_prefix string
    signatures []string
}

type steem_block struct {
    block_id string
    extensions []string
    previous string
    signing_key string
    timestamp string
    transaction_ids []string
    transaction_merkle_root string
    transactions []steem_transaction

}

type steem_result struct {
    block steem_block
}

type steem_http_result struct {
    id int64
    jsonrpc string
    result steem_result
}

func SaveFile(name,content string) {
    data := []byte(content)
    if ioutil.WriteFile(name,data,0644) != nil {
        fmt.Println("save data fail!")
    }
}

func get_block() {
    url:= "https://api.steem.fans"
    var data string
    var file_name string
    is_continue := true
    var steem_request_res steem_http_result

    for  is_continue {
        mutex.Lock()
        data =  "{\"jsonrpc\":\"2.0\",\"method\":\"block_api.get_block\",\"params\":{\"block_num\":" + strconv.FormatInt(block_count,10) + "},\"id\":1}"
        file_name = "blocks/block-" + strconv.FormatInt(block_count,10)
        block_count++
        mutex.Unlock()

        jsonStr :=[]byte(data)
        req, err := http.NewRequest("POST", url, bytes.NewBuffer(jsonStr))
        if err != nil {
            fmt.Println("NewRequest fail: ",err)
            continue
        }
        req.Header.Set("Content-Type", "application/json")
        client := &http.Client{}
        resp, err := client.Do(req)
        if err != nil {
            // handle error
            fmt.Println("clinet.Do fail: ",err)
            continue
        }
        defer resp.Body.Close()
        //statuscode := resp.StatusCode
        //hea := resp.Header
        body, _ := ioutil.ReadAll(resp.Body)
        //fmt.Println(string(body))
        json_err := json.Unmarshal(body,&steem_request_res)
        if json_err != nil {
            fmt.Println("json unmarshal fail: ",err)
            continue
        }
        is_continue = steem_request_res.result.block.block_id == ""
        if is_continue {
            SaveFile(file_name,string(body))
        }
        //fmt.Println(statuscode)
        //fmt.Println(hea)
    }
    defer wait.Done()
}

func get_block_s_e(start int64, end int64) {
    url:= "https://api.steem.fans"
    var data string
    var file_name string
    is_continue := true
    var steem_request_res steem_http_result

    for  i := start;i<=end;i++ {
        //mutex.Lock()
        data =  "{\"jsonrpc\":\"2.0\",\"method\":\"block_api.get_block\",\"params\":{\"block_num\":" + strconv.FormatInt(i,10) + "},\"id\":1}"
        file_name = "blocks/block-" + strconv.FormatInt(i,10)
        //block_count++
        //mutex.Unlock()

        jsonStr :=[]byte(data)
        req, err := http.NewRequest("POST", url, bytes.NewBuffer(jsonStr))
        if err != nil {
            fmt.Println("NewRequest fail: ",err)
            continue
        }
        req.Header.Set("Content-Type", "application/json")
        //req.Close = true
        client := &http.Client{}
        resp, err := client.Do(req)
        if err != nil {
            // handle error
            fmt.Println("clinet.Do fail: ",err)
            continue
        }
        defer resp.Body.Close()
        //statuscode := resp.StatusCode
        //hea := resp.Header
        body, _ := ioutil.ReadAll(resp.Body)
        //fmt.Println(string(body))
        json_err := json.Unmarshal(body,&steem_request_res)
        if json_err != nil {
            fmt.Println("json unmarshal fail: ",err)
            continue
        }
        is_continue = steem_request_res.result.block.block_id == ""
        if is_continue {
            SaveFile(file_name,string(body))
        }
        //fmt.Println(statuscode)
        //fmt.Println(hea)
    }
    defer wait.Done()
}

func main() {
    threading_num := 2
    wait.Add(threading_num)
    for i := 1;i <= threading_num;i++ {
        go get_block()
    }
    wait.Wait()
}