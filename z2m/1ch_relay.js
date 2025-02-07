const {onOff, deviceEndpoints, electricityMeter, identify,} = require("zigbee-herdsman-converters/lib/modernExtend");

const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
const ota = require("zigbee-herdsman-converters/lib/ota");
const utils = require("zigbee-herdsman-converters/lib/utils");
const globalStore = require("zigbee-herdsman-converters/lib/store");
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const e = exposes.presets;
const ea = exposes.access;

const definition = {
    zigbeeModel: ['esp32h2ch1'],
    model: 'esp32h2ch1',
    vendor: 'OMK',
    description: 'Custom FW',
    fromZigbee: [fz.electrical_measurement],
    // toZigbee: [tz.on_off, tz.identify],
    exposes: [e.power(),e.voltage(), e.current(), e.ac_frequency()],
    extend: [onOff({"powerOnBehavior":true, description: "Controls relay1"}), identify()],
    ota: ota.zigbeeOTA,
    configure: async (device, coordinatorEndpoint, logger) => {
        const endpoint = device.getEndpoint(1);
        await reporting.bind(endpoint, coordinatorEndpoint, ['haElectricalMeasurement']);
        // await reporting.onOff(endpoint);
        // await reporting.activePower(endpoint);
        // await reporting.rmsVoltage(endpoint);
        // await reporting.rmsCurrent(endpoint);
        utils.attachOutputCluster(device, 'genOta');
    },
};

module.exports = definition;