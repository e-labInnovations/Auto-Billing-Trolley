const path = require('path');
const { exec } = require('child_process');

const cmd = `node node_modules/node-red/red.js --userDir ${path.join(__dirname)}`;
exec(cmd, (err, stdout, stderr) => {
    if (err) {
        console.error(err);
        return;
    }
    console.log(stdout);
});
