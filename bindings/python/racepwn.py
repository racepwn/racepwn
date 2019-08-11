import race
import argparse
import json

from flask import Flask, jsonify, request, abort

app = Flask(__name__)

@app.route('/race', methods=['POST'])
def race_endpoint():
    print(request.json)
    if not request.json:
        abort(400)
    return jsonify(config_work(request.json)),200

def read_config(config_path):
    with open(config_path,'r') as f:
        return json.load(f)

def server_work(host, port):
    app.run(host=host, port=port)

def config_work(config_json):
    result = []
    for job in config_json:
        result.append(race.run_job(job))
    return result

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Racepwn python binding CLI')

    parser.add_argument('--hostname', type=str,
                        help='Setup host:port for server mode')
    parser.add_argument('-c', '--config', type=str,
                        help='Path to the config file.')

    args = parser.parse_args()
    if args.config is not None:
        print(config_work(read_config(args.config)))
    elif args.hostname is not None:
        host = args.hostname.split(':')
        server_work(host[0],host[1])
