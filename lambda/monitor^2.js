var AWS = require('aws-sdk');
AWS.config.update({region: 'us-east-1'});
var cloudwatch = new AWS.CloudWatch();

var alarm_metrics = [];
var global_context = {};

function alarmCallback(err, data, fetch) {
  var alarm_obj = {};
  if (!err) {
    if (data) {
      if (data.MetricAlarms) {
        alarm_metrics = alarm_metrics.concat(data.MetricAlarms);
      }
      if (data.NextToken) {
        alarm_obj.NextToken = data.NextToken;
        fetch = true;
      }
    }
    if (fetch) {
      cloudwatch.describeAlarms(alarm_obj, alarmCallback);
    } else {
      // TODO: push this back into a metric / alarm for alerting
      global_context.succeed("Found " + alarm_metrics.length + " alarms");
    }
  }
}

exports.handler = function(event, context) {
    global_context = context; // lazy
    alarmCallback(null, {}, true);
};

// for testing:
// exports.handler()
