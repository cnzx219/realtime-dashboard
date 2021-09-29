import redis
import time
import json
import random

def main():
    redis_client = redis.StrictRedis(host='127.0.0.1', port='6379', db='0', password='')
    v = 600.0
    while True:
        v += random.randrange(-100, 100) / 10
        redis_client.publish('testtopic', json.dumps({'myChart': [int(time.time()), v]}))
        time.sleep(1)


if __name__ == '__main__':
    main()
