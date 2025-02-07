const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const utils = require('zigbee-herdsman-converters/lib/utils');
const e = exposes.presets;
const ea = exposes.access;

const fzLocalElMes = {
    Electric_measurment: {
      cluster: "haElectricalMeasurement",
      type: ["attributeReport", "readResponse"],
      convert: (model, msg, publish, options, meta) => {
        const payload = {};
  
        if (msg.data.hasOwnProperty("ActivePower")) {
          payload.active_power_a = msg.data["ActivePower"];
        }
        if (msg.data.hasOwnProperty("activePowerPhB")) {
            payload.active_power_b = msg.data["activePowerPhB"];
        }
        if (msg.data.hasOwnProperty("rmsVoltage")) {
          payload.voltage_a = msg.data["rmsVoltage"];
        }
        if (msg.data.hasOwnProperty("rmsVoltagePhB")) {
          payload.voltage_b = msg.data["rmsVoltagePhB"];
        }
        if (msg.data.hasOwnProperty("rmsVoltagePhC")) {
          payload.voltage_c = msg.data["rmsVoltagePhC"];
        }
        if (msg.data.hasOwnProperty("rmsCurrent")) {
          payload.current_a = msg.data["rmsCurrent"];
        }
        if (msg.data.hasOwnProperty("rmsCurrentPhB")) {
          payload.current_b = msg.data["rmsCurrentPhB"];
        }
        if (msg.data.hasOwnProperty("rmsCurrentPhC")) {
          payload.current_c = msg.data["rmsCurrentPhC"];
        }
        return payload;
      },
    },
  };

const definition = {
    zigbeeModel: ['esp32h2ch1'],
    model: 'esp32h2ch1',
    vendor: 'OMK',
    description: 'Custom Zigbee device',
    fromZigbee: [fz.on_off, fzLocalElMes.Electric_measurment],
    toZigbee: [tz.on_off, tz.identify],
    meta: {multiEndpoint: true},
    configure: async (device, coordinatorEndpoint, logger) => {
        const endpoint1 = device.getEndpoint(1);
        const endpoint2 = device.getEndpoint(2);
        await endpoint1.read('genBasic', ['powerSource']);
        // await endpoint.read('genBasic', ['manufacturerName', 'zclVersion', 'appVersion', 'modelId', 'powerSource', 0xfffe]);
        await reporting.bind(endpoint1, coordinatorEndpoint, ['genOnOff', 'haElectricalMeasurement']);
        await reporting.bind(endpoint2, coordinatorEndpoint, ['genOnOff']);
        await reporting.onOff(endpoint1);
        await reporting.onOff(endpoint2);
    },
    exposes: [
        e.switch().withEndpoint('1'),
        e.switch().withEndpoint('2'),
        e.identify(),
        e.numeric("voltage_a", ea.STATE).withUnit("V").withDescription("Phase A Voltage"),
        e.numeric("current_a", ea.STATE).withUnit("A").withDescription("Phase A Current"),
        e.numeric("active_power_a", ea.STATE).withUnit("W").withDescription("Power CH1"),
        e.numeric("voltage_b", ea.STATE).withUnit("V").withDescription("Phase B Voltage"),
        e.numeric("current_b", ea.STATE).withUnit("A").withDescription("Phase B Current"),
        e.numeric("active_power_b", ea.STATE).withUnit("W").withDescription("Power CH1"),
    ],
};

module.exports = definition;