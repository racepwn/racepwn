import librace


class RaceRawParam:
    def __init__(self, data, count):
        self.data = data
        self.count = count


class RaceParam:
    def __init__(self, type, delay_time_usec, last_chunk_size):
        self.type = type
        self.delay_time_usec = delay_time_usec
        self.last_chunk_size = last_chunk_size


class RaceRaw:
    def __init__(self, host, ssl_flag, race_param):
        self.host = host
        self.ssl_flag = ssl_flag
        self.race_param = race_param


class RaceJob:
    def __init__(self, race_param, raw_param):
        self.race_param = race_param
        self.raw_param = raw_param


def parse_config(job_json):
    try:
        race_j = job_json["race"]
        raw_j = job_json["raw"]
        race_p = RaceParam(race_j["type"], race_j["delay_time_usec"], race_j["last_chunk_size"])

        raw_param_array = [RaceRawParam(raw_p["data"], raw_p["count"]) for raw_p in raw_j["race_param"]]
        raw_p = RaceRaw(raw_j["host"], raw_j["ssl"], raw_param_array)

        return RaceJob(race_p, raw_p)
    except:
        return None


def run_job(job):
    def free():
        try:
            librace.race_free(race_session)
            librace.race_raw_free(race_raw_session)
        except:
            pass

    job = parse_config(job)
    if job is None:
        return "Job parsing error. Check config file!"

    race_param = job.race_param
    raw_param = job.raw_param
    race_session = librace.race_new()

    if race_param.type == "paralell":
        librace.race_set_option(race_session, librace.RACE_OPT_MODE_PARALELLS, 1)
    elif race_param.type == "pipeline":
        librace.race_set_option(race_session, librace.RACE_OPT_MODE_PIPELINE, 1)

    librace.race_set_option(
        race_session,
        librace.RACE_OPT_LAST_CHUNK_DELAY_USEC,
        librace.c_size_t(race_param.delay_time_usec),
    )
    librace.race_set_option(
        race_session,
        librace.RACE_OPT_LAST_CHUNK_SIZE,
        librace.c_size_t(race_param.last_chunk_size),
    )

    race_raw_session = librace.race_raw_new(race_session)

    librace.race_raw_set_url(
        race_raw_session,
        librace.String(raw_param.host),
        librace.c_bool(raw_param.ssl_flag),
    )

    for param in raw_param.race_param:
        librace.race_raw_add_race_param(
            race_raw_session,
            librace.c_uint(param.count),
            librace.c_char_p(param.data.encode("utf-8")),
            librace.c_size_t(len(param.data) + 1),
        )

    librace.race_raw_apply(race_raw_session)

    race_info = librace.pointer(librace.race_info_t())

    librace.race_run(race_session, librace.pointer(race_info))

    count = librace.race_info_count(race_info)
    resp_list = []
    for index in range(count):
        resp = librace.race_info_get_response(race_info, librace.c_uint(index))
        resp_list.append(librace.string_at(resp.data, resp.len).decode("utf-8"))

    return resp_list
