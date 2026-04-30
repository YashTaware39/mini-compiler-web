document.addEventListener('DOMContentLoaded', () => {
    const runBtn = document.getElementById('runBtn');
    const codeInput = document.getElementById('codeInput');
    const tokensOut = document.getElementById('tokensOutput');
    const parserOut = document.getElementById('parserOutput');
    const symbolOut = document.getElementById('symbolOutput');

    runBtn.addEventListener('click', async () => {
        const code = codeInput.value.trim();
        if (!code) {
            alert('Please enter some code.');
            return;
        }

        // Loading state
        runBtn.innerHTML = 'Running...';
        runBtn.disabled = true;

        try {
            const response = await fetch('http://localhost:3000/run', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ code })
            });

            const data = await response.json();

            if (data.error) {
                tokensOut.innerHTML = 'Error';
                parserOut.innerHTML = `<span class="error-text">${data.error}</span>`;
                symbolOut.innerHTML = 'Error';
            } else if (data.result) {
                parseAndDisplayResult(data.result);
            } else {
                parserOut.innerHTML = `<span class="error-text">Unexpected response format</span>`;
            }

        } catch (err) {
            console.error(err);
            parserOut.innerHTML = `<span class="error-text">Failed to connect to backend server. Make sure it is running.</span>`;
        } finally {
            // Restore button
            runBtn.innerHTML = `<svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><path d="M8 5V19L19 12L8 5Z" fill="currentColor"/></svg> Run Compiler`;
            runBtn.disabled = false;
        }
    });

    function parseAndDisplayResult(rawText) {
        // We know the C app prints these exact headers:
        // --- TOKENS ---
        // ...
        // --- PARSING RESULT ---
        // ...
        // --- SYMBOL TABLE ---
        // ...

        let tokensPart = "";
        let parserPart = "";
        let symbolPart = "";

        const lines = rawText.split('\n');
        
        let currentSection = 0; // 0=start, 1=tokens, 2=parser, 3=symtab

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i].trim();
            if (line === '--- TOKENS ---') { currentSection = 1; continue; }
            if (line === '--- PARSING TRACE ---') { currentSection = 2; parserPart += '--- PARSING TRACE ---\n'; continue; }
            if (line === '--- SYMBOL TABLE ---') { currentSection = 3; continue; }

            if (currentSection === 1) tokensPart += lines[i] + '\n';
            else if (currentSection === 2) parserPart += lines[i] + '\n';
            else if (currentSection === 3) symbolPart += lines[i] + '\n';
        }

        // Inject content
        tokensOut.innerHTML = formatTokensOutput(tokensPart || "No tokens.");
        parserOut.innerHTML = formatParserOutput(parserPart || "No parsing logic available.");
        symbolOut.innerHTML = formatTokensOutput(symbolPart || "(empty)");
    }

    function formatTokensOutput(text) {
        // Just escape HTML
        return text.replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }

    function formatParserOutput(text) {
        let esc = text.replace(/</g, "&lt;").replace(/>/g, "&gt;");
        esc = esc.replace(/Parsing Completed Successfully\./g, '<span class="success-text">Parsing Completed Successfully.</span>');
        esc = esc.replace(/(Syntax Error at line \d+:.*)/g, '<span class="error-text">$1</span>');
        esc = esc.replace(/--- PARSING TRACE ---/g, '<span style="color:#61afef; font-weight:bold">--- PARSING TRACE ---</span>');
        esc = esc.replace(/--- GRAMMAR RULES ---/g, '<span style="color:#61afef; font-weight:bold">--- GRAMMAR RULES ---</span>');
        esc = esc.replace(/--- RESULT ---/g, '<span style="color:#c678dd; font-weight:bold">--- RESULT ---</span>');
        esc = esc.replace(/--- SUMMARY ---/g, '<span style="color:#56b6c2; font-weight:bold">--- SUMMARY ---</span>');
        return esc;
    }
});
