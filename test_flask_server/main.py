#!flask/bin/python
from flask import Flask

app = Flask(__name__)

@app.route('/')
def index():
    return "Hello, World!"
@app.after_request
def after(response):
    print(response.status)
    print(response.headers)
    print(response.get_data())
    return response
if __name__ == '__main__':
    app.run(debug=True)
