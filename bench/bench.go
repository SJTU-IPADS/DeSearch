// Fetchall fetches URLs in parallel and reports their times and sizes.
package main

import(
    "fmt"
    "io"
    "io/ioutil"
    "net/http"
    "time"
    "flag"
    "math/rand"
    "strings"
    "log"
    
    "github.com/jamiealquiza/tachymeter"
//    "strconv"
)

var (
    request_number = flag.Int("requests", 1000, "number of requests")
    keyword_number = flag.Int("keywords", 1, "number of keywords")
)

func main() {

    config := tachymeter.New(&tachymeter.Config{Size: 50})

    // for parallel
    // runtime.GOMAXPROCS(16)

    // read entire keyword list into memory
    content, err := ioutil.ReadFile("./top-10k-words.txt")
    if err != nil {
        log.Fatal(err)
    }
    text := string(content)
    keywords := strings.Split(text, "\n")

    // random shuffle
    rand.Seed(time.Now().UnixNano())
    for i := len(keywords) - 1; i > 0; i-- { // Fisher–Yates shuffle
        j := rand.Intn(i + 1)
        keywords[i], keywords[j] = keywords[j], keywords[i]
    }

    // NOTE: add the list of all the queriers below
    // otherwise you are just benchmarking the local web server
    urls := []string{
        // "http://localhost:12000" + "/query/",
        }

    benchmark_start := time.Now()
    ch := make(chan string)
    for i := 0;i < *request_number; i++ {

        keyword_list := string("")
        for j := 0; j < (*keyword_number-1); j++ {
            keyword_list += keywords[(i+j)%10000] + "+"
        }
        keyword_list += keywords[(i+(*keyword_number))%10000]

        url := urls[i%len(urls)] + keyword_list + "/page/1"
        go func (url string, ch chan<- string) { // start a goroutine
            start := time.Now()
            resp, err := http.Get(url)
            if err != nil {
                ch <- fmt.Sprint(err) // send to channel ch
                return
            }

            nbytes, err := io.Copy(ioutil.Discard, resp.Body)
            resp.Body.Close() // don't leak resources
            if err != nil {
                ch <- fmt.Sprintf("while reading %s: %v", url, err)
                return
            }
            config.AddTime(time.Since(start))
            secs := time.Since(start).Seconds()
            ch <- fmt.Sprintf("%.2fs  %7d  %s", secs, nbytes, url)
        } (url, ch)
    }
    for i := 0;i < *request_number; i++ {
        fmt.Println(<-ch) // receive from channel ch
        //<-ch
    }
    elapsed := time.Since(benchmark_start)
    fmt.Printf("%.2fs elapsed\n", elapsed.Seconds())
    fmt.Printf("Throughput: %f ops/sec\n", float64(*request_number) / elapsed.Seconds())

	// Calc output.
	results := config.Calc()

	// Print JSON format to console.
	fmt.Printf("%s\n\n", results.JSON())

	// Print pre-formatted console output.
	fmt.Printf("%s\n\n", results)

	// Print text histogram.
	fmt.Println(results.Histogram.String(15))
}
