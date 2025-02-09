const {deviceEndpoints, identify, battery} = require("zigbee-herdsman-converters/lib/modernExtend");

const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
// const m = require("zigbee-herdsman-converters/lib/modernExtend");
const ota = require("zigbee-herdsman-converters/lib/ota");
const utils = require("zigbee-herdsman-converters/lib/utils");
const globalStore = require("zigbee-herdsman-converters/lib/store");
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const e = exposes.presets;
const ea = exposes.access;

const airWick ={
    key: ['airWick'],
    convertSet: async (entity, key, value, meta) => {
        await entity.command('genOnOff', 'toggle', {}, utils.getOptions(meta.mapped, entity));
        // await entity.write('genOnOff', {0: {value: 1, type: 0x20}}, utils.getOptions(meta.mapped, entity));
    },
};

const definition = {
    zigbeeModel: ['esp32AirWick'],
    model: 'esp32AirWick',
    vendor: 'OMK',
    description: 'Custom FW',
    toZigbee: [airWick],
    fromZigbee:[fz.battery],
    exposes: [exposes.enum('airWick', ea.SET, ['push']).withDescription('Spray action for AirWick device'), e.battery()],
    // extend: [battery()],
    ota: ota.zigbeeOTA,
    configure: async (device, coordinatorEndpoint) => {
        const endpoint = device.getEndpoint(1);
        await reporting.bind(endpoint, coordinatorEndpoint, ['genPowerCfg']);
        await reporting.batteryPercentageRemaining(endpoint);
        utils.attachOutputCluster(device, 'genOta');
    },
};

module.exports = definition;


