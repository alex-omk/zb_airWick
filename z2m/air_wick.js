const {deviceEndpoints, identify,} = require("zigbee-herdsman-converters/lib/modernExtend");

const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
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
    zigbeeModel: ['esp32h2AirWick'],
    model: 'esp32h2AirWick',
    vendor: 'OMK',
    description: 'Custom FW',
    toZigbee: [airWick],
    exposes: [exposes.enum('airWick', ea.SET, ['push']).withDescription('Spray action for AirWick device'),],
    // exposes: [],
    extend: [identify()],
    ota: ota.zigbeeOTA,
    configure: async (device, coordinatorEndpoint, logger) => {
        const endpoint = device.getEndpoint(1);
        // await reporting.bind(endpoint, coordinatorEndpoint, ['haElectricalMeasurement']);
        // await reporting.onOff(endpoint);
        // await reporting.activePower(endpoint);
        // await reporting.rmsVoltage(endpoint);
        // await reporting.rmsCurrent(endpoint);
        utils.attachOutputCluster(device, 'genOta');
    },
};

module.exports = definition;

// const definition = {
//     zigbeeModel: ['esp32h2AirWick'],
//     model: 'esp32h2AirWick',
//     vendor: 'OMK',
//     description: 'Custom FW',
//     fromZigbee: [], // Замените на кастомные обработчики, если нужно
//     toZigbee: [{
//         key: ['airWick'], // Изменено на 'airWick' для соответствия экспозиции
//         convertSet: async (entity, key, value, meta) => {
//             // Отправка команды в кастомный кластер 0xFFF2
//             await entity.write(
//                 65522,
//                 '0',
//                 {value: 1},
//                 utils.getOptions(meta)
//             );
//         },
//     }],
//     exposes: [
//         exposes.enum('airWick', ea.SET, ['push']).withDescription('Spray'), // Совпадает с key в toZigbee
//     ],
// };

// module.exports = definition;


// const definition = {
//     zigbeeModel: ['esp32h2AirWick'], // Название модели Zigbee устройства
//     model: 'esp32h2AirWick',
//     vendor: 'OMK',
//     description: 'Custom FW',
    
//     fromZigbee: [fz.command_on, fz.command_off], // Обработка входящих сообщений (настройте при необходимости)
    
//     toZigbee: [{
//         key: ['airWick'], // Это ключ, по которому будет определяться команда из MQTT
//         convertSet: async (entity, key, value, meta) => {
//             if (value === 'push') {
//                 // Отправка команды в кастомный кластер (Cluster ID 65522)
//                 await entity.command(
//                     65522, // Cluster ID в числовом формате (десятичный)
//                     0x00, // Команда, которую нужно отправить
//                     {customData: 1}, // Полезная нагрузка, передаем '1' при нажатии
//                     utils.getOptions(meta) // Опции для передачи
//                 );
//             } else {
//                 throw new Error(`Invalid value for airWick: ${value}`);
//             }
//         },
//     }],
    
//     exposes: [
//         exposes.enum('airWick', ea.SET, ['push'])
//             .withDescription('Spray action for AirWick device'),
//     ],
// };

// module.exports = definition;



// const fLocal = {
//     airWick: {
//       cluster: '65522', //This part is important
//       key: ['push'],
//       convertSet: async (entity, key, value, meta) => {
//         // External value takes priority over options for compatibility
//         const identifyTimeout = 33;
//         await entity.command(cluster, '0', {identifytime: identifyTimeout}, utils.getOptions(meta.mapped, entity));
//     },
// 	//   type: ['readResponse', 'attributeReport'],
// 	//   convert: (model, msg, publish, options, meta) => {
//     //     return {action: postfixWithEndpointName(`push`, msg, model, meta)};
//     //   },
//     },
// };


