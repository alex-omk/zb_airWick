const {deviceEndpoints, identify, battery} = require("zigbee-herdsman-converters/lib/modernExtend");

const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
// const m = require("zigbee-herdsman-converters/lib/modernExtend");
const ota = require("zigbee-herdsman-converters/lib/ota");
const utils = require("zigbee-herdsman-converters/lib/utils");
const globalStore = require("zigbee-herdsman-converters/lib/store");
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const logger = require('zigbee-herdsman-converters/lib/logger');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const e = exposes.presets;
const ea = exposes.access;

// }; sihas_set_people


const tzLocal_AnalogCluster = {
    sprayInterval: {
        key: ["sprayInterval"],
        convertSet: async (entity, key, value, meta) => {
            const payload = {presentValue: value};
            const endpoint = meta.device.endpoints.find((e) => e.supportsInputCluster('genAnalogOutput'));
            await endpoint.write('genAnalogOutput', payload);
        },
        convertGet: async (entity, key, meta) => {
            const endpoint = meta.device.endpoints.find((e) => e.supportsInputCluster('genAnalogOutput'));
            await endpoint.read('genAnalogOutput', ['presentValue']);
        },
    },
};

const fzLocal_AnalogCluster = {
    sprayInterval: {
        cluster: 'genAnalogOutput',
        type: ["attributeReport", 'readResponse'],
        convert: (model, msg, publish, options, meta) => {
            // const value = precisionRound(msg.data.presentValue, 0);
            const payload = {};
            if (msg.data.hasOwnProperty("presentValue")) {
                payload.sprayInterval = msg.data["presentValue"];
            }
            return payload;
        },
    },
    analogInput: {
        cluster: 'genAnalogInput',
        type: ["attributeReport", 'readResponse'],
        convert: (model, msg, publish, options, meta) => {
            const payload = {};
            if (msg.data.hasOwnProperty("presentValue")) {
                meta.logger.info(`+_+_+_ fromZigbeeConverter() msg.endpoint=[${JSON.stringify(msg.endpoint)}]`);
                // meta.logger.debug(`+_+_+_ fromZigbeeConverter() model=[${JSON.stringify(model)}]`);
                // meta.logger.debug(`+_+_+_ fromZigbeeConverter() msg=[${JSON.stringify(msg)}]`);
                if (msg.endpoint.ID === 1){
                    payload.sprayCounter = msg.data["presentValue"];
                }
                if (msg.endpoint.ID === 11){
                    if (msg.data["presentValue"] !== 6565){
                        payload.batteryVoltage = msg.data["presentValue"];
                    }
                }
            }
            return payload;
        },
    },
    // batteryVoltage: {
    //     cluster: 'genAnalogValue',
    //     type: ["attributeReport", 'readResponse'],
    //     convert: (model, msg, publish, options, meta) => {
    //         const payload = {};
    //         if (msg.data.hasOwnProperty("presentValue")) {
    //             payload.batteryVoltage = msg.data["presentValue"];
    //         }
    //         return payload;
    //     },
    // },
};

const airWick ={
    key: ['airWick'],
    convertSet: async (entity, key, value, meta) => {
        await entity.command('genOnOff', 'toggle', {}, utils.getOptions(meta.mapped, entity));
    },
};

const definition = {
    zigbeeModel: ['esp32AirWick'],
    model: 'esp32AirWick',
    vendor: 'OMK',
    description: 'Custom FW',
    toZigbee: [airWick, tz.factory_reset, tzLocal_AnalogCluster.sprayInterval],
    fromZigbee:[fz.battery, fzLocal_AnalogCluster.analogInput, fzLocal_AnalogCluster.sprayInterval,],
    meta: {multiEndpoint: true},
    exposes: [
        e.enum('airWick', ea.SET, ['Spray']).withDescription('Spray action for AirWick device'), 
        // e.battery(),
        // e.battery_voltage(), 
        e.enum('reset', ea.SET, ['Push']).withDescription("Resets the spray counter"),
        e.numeric('sprayInterval', ea.ALL)
            .withValueMin(0)
            .withValueMax(1440)
            .withUnit('min')
            .withDescription('Sets the spray interval in minutes; if set to 0, auto-spray is disabled'),
        e.numeric("sprayCounter", ea.STATE).withDescription("Displays the total number of sprays"),
        e.numeric("batteryVoltage", ea.STATE).withUnit('mV').withDescription("Battery voltage in millivolts"),
    ],
    extend: [battery({"percentage":true, "voltage":false, "percentageReporting":false})],
    ota: ota.zigbeeOTA,
    configure: async (device, coordinatorEndpoint, logger) => {
        const endpoint = device.getEndpoint(1);
        const endpoint11 = device.getEndpoint(11);
        await reporting.bind(endpoint, coordinatorEndpoint, ['genPowerCfg', 'genAnalogInput', 'genAnalogOutput']);
        await reporting.bind(endpoint11, coordinatorEndpoint, ['genAnalogInput',]);
        // await reporting.batteryPercentageRemaining(endpoint);
        await endpoint.read('genPowerCfg', ['batteryVoltage']);
        await endpoint.read('genAnalogValue', ['presentValue']);
        await endpoint.read('genAnalogInput', ['presentValue']);
        await endpoint11.read('genAnalogInput', ['presentValue']);
        utils.attachOutputCluster(device, 'genOta');
    },
};

module.exports = definition;


// const payloadDiagnostic = [
//     {
//         attribute: 'numberOfResets',
//         minimumReportInterval: 5,
//         maximumReportInterval: 60,
//         reportableChange: 0,
//     }
// ];
// // await endpoint.configureReporting('haDiagnostic', payloadDiagnostic);