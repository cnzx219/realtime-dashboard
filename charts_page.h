//
// Created by Hiro on 2021/12/22.
//

#ifndef REALTIME_DASHBOARD_BACKEND_CHARTS_PAGE_H
#define REALTIME_DASHBOARD_BACKEND_CHARTS_PAGE_H

#define CHARTS_PAGE_HTML_SOURCE R"(
<!DOCTYPE html>
<html style="height: 100%">
    <head>
        <meta charset="utf-8">
    </head>
    <body style="height: 100%; margin: 0">
        <div id="container" style="height: 100%"></div>

        <script src="https://cdn.bootcdn.net/ajax/libs/echarts/5.1.2/echarts.min.js"></script>

        <script type="text/javascript">
var dom = document.getElementById("container");
var myChart = echarts.init(dom);
var app = {};
var option;
let data = [];

option = {
  title: {
    text: 'Realtime Chart'
  },
  tooltip: {
    trigger: 'axis',
    axisPointer: {
      animation: false
    }
  },
  xAxis: {
    type: 'time',
    splitLine: {
      show: false
    }
  },
  yAxis: {
    type: 'value',
    boundaryGap: [0, '100%'],
    splitLine: {
      show: false
    }
  },
  series: [
    {
      name: 'Data',
      type: 'line',
      showSymbol: false,
      data: data
    }
  ]
};

const chartName = 'myChart';
var ws = new WebSocket('ws://localhost:3000/ws');
var initlen = 50;
ws.onopen = function () {
    ws.send('the request from client');
};
ws.onmessage = function (e) {
    const payload = JSON.parse(e.data);
    if (payload.type) {
        if (payload.type === 'init') {
            initlen = payload.initlen;
            for (const item of payload.data) {
                const currentDate = new Date(item[chartName][0] * 1000);
                data.unshift({
                    name: currentDate.toString() ,
                    value: [currentDate.getTime(), item[chartName][1]],
                });
            }

            myChart.setOption({
                series: [
                    {
                        data: data
                    }
                ]
            });
        }
    } else {
        const currentDate = new Date(payload[chartName][0] * 1000);

        data.push({
            name: currentDate.toString() ,
            value: [currentDate.getTime(), payload[chartName][1]],
        });
        if (data.length > initlen) {
            data.shift();
        }

        myChart.setOption({
            series: [
                {
                    data: data
                }
            ]
        });
    }

};

if (option && typeof option === 'object') {
    myChart.setOption(option);
}

        </script>
    </body>
</html>
)"

#endif //REALTIME_DASHBOARD_BACKEND_CHARTS_PAGE_H
