const express = require('express');
const cors = require('cors');
const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

const app = express();
app.use(cors());
app.use(express.json());

// The executable path
const COMPILER_EXE = process.platform === 'win32' 
    ? path.join(__dirname, '..', 'compiler', 'compiler.exe')
    : path.join(__dirname, '..', 'compiler', 'compiler');

app.post('/run', (req, res) => {
    const code = req.body.code;
    if (!code) {
        return res.status(400).json({ error: "No code provided" });
    }

    if (!fs.existsSync(COMPILER_EXE)) {
        return res.status(500).json({ error: "Compiler executable not found. Please compile the C code first." });
    }

    const compiler = spawn(COMPILER_EXE);

    let output = '';
    let errorOutput = '';

    compiler.stdout.on('data', (data) => {
        output += data.toString();
    });

    compiler.stderr.on('data', (data) => {
        errorOutput += data.toString();
    });

    compiler.on('close', (code) => {
        // compiler exits, return full output
        if (code !== 0 || errorOutput) {
            return res.json({ result: output + "\n" + errorOutput });
        }
        res.json({ result: output });
    });

    // Send the user input multi-line code to compiler's standard input
    compiler.stdin.write(code);
    compiler.stdin.end();
});

const PORT = 3000;
app.listen(PORT, () => {
    console.log(`Backend server running on http://localhost:${PORT}`);
});
