package race

/*
#cgo LDFLAGS: -L${SRCDIR}/../../build/lib/ -lrace -levent -lssl -lcrypto
#include "../../lib/include/race.h"
#include <stdlib.h>
*/
import "C"
import (
	"errors"
	"unsafe"
)

type raceRawParam struct {
	Data  string `json:"data"`
	Count uint   `json:"count"`
}

type raceParam struct {
	Type          string `json:"type"`
	DelayTimeUsec uint   `json:"delay_time_usec"`
	LastChunkSize uint   `json:"last_chunk_size"`
}

type raceRaw struct {
	Host      string         `json:"host"`
	SSLFlag   bool           `json:"ssl"`
	RaceParam []raceRawParam `json:"race_param"`
}

type RaceJob struct {
	Type string    `json:"body_type"`
	Race raceParam `json:"race"`
	Raw  raceRaw   `json:"raw"`
}

func Run(rJob *RaceJob) error {
	if rJob == nil {
		return errors.New("empty job")
	}
	cRace := C.race_job_new()
	rParam := &rJob.Race
	if rParam.Type == "paralell" {
		C.race_set_option(cRace, 0, 1)
	} else if rParam.Type == "pipeline" {
		C.race_set_option(cRace, 1, 1)
	} else {
		return errors.New("race type must be specified")
	}
	C.race_set_option(cRace, 2, C.size_t(rParam.DelayTimeUsec))
	C.race_set_option(cRace, 3, C.size_t(rParam.LastChunkSize))
	if rJob.Type == "raw" {
		rRaw := &rJob.Raw
		cRaw := C.rraw_new()
		C.rraw_set_host(cRaw, C.CString(rRaw.Host), C._Bool(rRaw.SSLFlag))
		for _, raceParam := range rRaw.RaceParam {
			C.rraw_set_race_param(cRaw, C.uint(raceParam.Count), unsafe.Pointer(C.CString(raceParam.Data)), C.size_t(len(raceParam.Data)))
		}
		C.rraw_set_race(cRace, cRaw)
	} else {
		return errors.New("race body must be specified")
	}
	C.race_run(cRace)
	return nil
}
