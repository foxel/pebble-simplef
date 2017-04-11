var clayConfig = [
    {
        "type": "heading",
        "defaultValue": "Configuration"
    },
    {
        "type": "toggle",
        "messageKey": "INVERSE",
        "label": "Inverse colors",
        "defaultValue": false
    },
    {
        "type": "submit",
        "defaultValue": "Save Settings"
    }
];

module.exports = function() {
    var Clay = require('pebble-clay');
    return new Clay(clayConfig);
}
