package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"

	"./race"
)

var hostname = flag.String("hostname", "", "Run util as server (need specify address)")

func main() {
	flag.Parse()
	if *hostname != "" {
		racepwnRunAsServer(*hostname)
	} else {
		jobStatusJSON := racepwnRun(os.Stdin)
		os.Stdout.Write(jobStatusJSON)
	}
}

func racepwnRunAsServer(hostname string) {
	http.HandleFunc("/race", func(w http.ResponseWriter, r *http.Request) {
		defer r.Body.Close()
		w.Write(racepwnRun(r.Body))
	})
	if err := http.ListenAndServe(hostname, nil); err != nil {
		log.Fatal("ListenAndServe: ", err)
	}
}

func racepwnRun(r io.Reader) []byte {
	decoder := json.NewDecoder(r)
	var statuses []race.RaceStatus
	var jobs []race.RaceJob
	if err := decoder.Decode(&jobs); err != nil {
		fmt.Fprintln(os.Stderr, "cannot read from stdin:", err)
		return nil
	}
	for _, job := range jobs {
		raceStatus, err := race.Run(&job)
		if err != nil {
			fmt.Fprintln(os.Stderr, "cannot apply race job:", err)
			return []byte(err.Error())
		}
		statuses = append(statuses, *raceStatus)
	}
	statusesJSON, err := json.Marshal(statuses)
	if err != nil {
		fmt.Fprintln(os.Stderr, "cannot marshal json:", err)
		return []byte(err.Error())
	}
	return statusesJSON
}
