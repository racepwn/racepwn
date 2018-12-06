package race

/*
#cgo LDFLAGS: -L${SRCDIR}/../../build/lib/ -lracestatic -levent -levent_openssl -lssl -lcrypto
#include "../../lib/include/race.h"
#include "../../lib/include/race_raw.h"
#include "../../lib/include/race_status.h"
#include <stdlib.h>
#include <string.h>
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
	DelayTimeUsec int    `json:"delay_time_usec"`
	LastChunkSize int    `json:"last_chunk_size"`
}

type raceRaw struct {
	Host      string         `json:"host"`
	SSLFlag   bool           `json:"ssl"`
	RaceParam []raceRawParam `json:"race_param"`
}

type RaceJob struct {
	Race raceParam `json:"race"`
	Raw  *raceRaw  `json:"raw"`
}

type RaceResponse struct {
	Responce string `json:"response"`
}

type RaceStatus struct {
	Responses []RaceResponse `json:"responses"`
}

func Run(rJob *RaceJob) (*RaceStatus, error) {
	raceStatus := &RaceStatus{}
	if rJob == nil {
		return nil, errors.New("empty job")
	}
	cRace := C.race_new()
	defer C.race_free(cRace)
	rParam := &rJob.Race
	if rParam.Type == "paralell" {
		C.race_set_option(cRace, C.RACE_OPT_MODE_PARALELLS, 1)
	} else if rParam.Type == "pipeline" {
		C.race_set_option(cRace, C.RACE_OPT_MODE_PIPELINE, 1)
	}
	if C.race_set_option(cRace, C.RACE_OPT_LAST_CHUNK_DELAY_USEC, C.size_t(rParam.DelayTimeUsec)) != C.RACE_OK {
		return nil, errors.New(C.GoString(C.race_strerror(cRace)))
	}
	if C.race_set_option(cRace, C.RACE_OPT_LAST_CHUNK_SIZE, C.size_t(rParam.LastChunkSize)) != C.RACE_OK {
		return nil, errors.New(C.GoString(C.race_strerror(cRace)))
	}
	if rJob.Raw != nil {
		rRaw := rJob.Raw
		cRaw := C.race_raw_new(cRace)
		defer C.race_raw_free(cRaw)
		host := C.CString(rRaw.Host)
		if C.race_raw_set_url(cRaw, host, C._Bool(rRaw.SSLFlag)) != C.RACE_OK {
			C.free(unsafe.Pointer(host))
			return nil, errors.New(C.GoString(C.race_strerror(cRace)))
		}
		C.free(unsafe.Pointer(host))
		for _, raceParam := range rRaw.RaceParam {
			data := C.CString(raceParam.Data)
			len := C.strlen(data) + 1
			if C.race_raw_add_race_param(cRaw, C.uint(raceParam.Count), unsafe.Pointer(data), len) != C.RACE_OK {
				C.free(unsafe.Pointer(data))
				return nil, errors.New(C.GoString(C.race_strerror(cRace)))
			}
			C.free(unsafe.Pointer(data))
		}
		if C.race_raw_apply(cRaw) != C.RACE_OK {
			return nil, errors.New(C.GoString(C.race_strerror(cRace)))
		}
	} else {
		return nil, errors.New("race body must be specified")
	}
	cRaceInfo := &C.race_info_t{}
	if C.race_run(cRace, &cRaceInfo) != C.RACE_OK {
		defer C.race_info_free(cRaceInfo)
		return nil, errors.New(C.GoString(C.race_strerror(cRace)))
	}
	defer C.race_info_free(cRaceInfo)
	count := C.race_info_count(cRaceInfo)
	for index := 0; index < int(count); index++ {
		resp := C.race_info_get_response(cRaceInfo, C.uint(index))
		respBuf := C.GoStringN((*C.char)(resp.data), C.int(resp.len))
		raceStatus.Responses = append(raceStatus.Responses, RaceResponse{respBuf})
	}
	return raceStatus, nil
}
