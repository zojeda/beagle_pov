
var fs = require('fs');
var argv = require('minimist')(process.argv.slice(2));
var NR_LEDS = 20;
var pixel_values = new Buffer(NR_LEDS*3);

var pos = 0;
if(argv.pos) {
    pos = argv.pos;
}

if(argv.clear) {
    pos = NR_LEDS;
}

for(var i=0; i<NR_LEDS; i++) {
    if(i!=pos) {
        pixel_values.writeUInt8(0, 3*i);
        pixel_values.writeUInt8(0, 3*i+1);
        pixel_values.writeUInt8(0, 3*i+2);
    } else {
        toHexColor(argv.colors, 3*i);
    }

}

var wstream = fs.createWriteStream('/dev/beaglepov', {encoding: 'binary'});
wstream.write(pixel_values);
wstream.end();


var color_re = /#/

function toHexColor(colors, pos) {
    console.log(colors);
    var result = "";
        var m = colors.match(/^#([0-9a-f]{6})$/i)[1]; 
        if(m) {
            var red = parseInt(m.substr(0,2),16);
            var green = parseInt(m.substr(2,2),16);
            var blue = parseInt(m.substr(4,2),16);
            console.log(red, green, blue); 
            pixel_values.writeUInt8(red, pos);
            pixel_values.writeUInt8(green, pos+1);
            pixel_values.writeUInt8(blue, pos+2);
        } else {
            result += '\x00\x00\x00';
        }
    console.log(result);
    return result;
}
