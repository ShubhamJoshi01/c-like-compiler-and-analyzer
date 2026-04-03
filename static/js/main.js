document.addEventListener('DOMContentLoaded', () => {
    // Initialize CodeMirror with dracula theme
    const codeEditor = CodeMirror.fromTextArea(document.getElementById('code-editor'), {
        mode: 'text/x-csrc',
        theme: 'dracula',
        lineNumbers: true,
        indentUnit: 4,
        matchBrackets: true,
        autoCloseBrackets: true
    });

    const analyzeBtn = document.getElementById('analyze-btn');
    const tokensBody = document.getElementById('tokens-table-body');
    const issuesList = document.getElementById('issues-list');
    const aiSummaryText = document.getElementById('ai-summary-text');
    const summaryPanel = document.getElementById('summary-panel');
    const terminalOutput = document.getElementById('terminal-output');

    // Metrics elements
    const metricTokens = document.getElementById('metric-tokens');
    const metricIssues = document.getElementById('metric-issues');
    const metricFuncs = document.getElementById('metric-funcs');
    const metricVars = document.getElementById('metric-vars');
    const scoreBar = document.getElementById('score-bar');
    const scoreText = document.getElementById('score-text');

    // Tabs logic
    const tabBtns = document.querySelectorAll('.g-tab');
    const tabPanes = document.querySelectorAll('.g-pane');

    tabBtns.forEach(btn => {
        btn.addEventListener('click', () => {
            tabBtns.forEach(b => b.classList.remove('active'));
            tabPanes.forEach(p => p.classList.remove('active'));
            
            btn.classList.add('active');
            document.getElementById(btn.dataset.tab).classList.add('active');
        });
    });

    analyzeBtn.addEventListener('click', async () => {
        const code = codeEditor.getValue();
        analyzeBtn.innerHTML = 'Analyzing... <span style="display:inline-block; animation: spin 1s linear infinite;">⏳</span>';
        analyzeBtn.disabled = true;

        try {
            const response = await fetch('/analyze', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ code })
            });

            const data = await response.json();
            renderResults(data);
        } catch (error) {
            console.error("Error analyzing code:", error);
            issuesList.innerHTML = `<div class="glass-panel" style="border-left: 4px solid var(--error-color)"><p>Network error or backend issue occurred.</p></div>`;
        } finally {
            analyzeBtn.innerHTML = 'Analyze Code <span>✨</span>';
            analyzeBtn.disabled = false;
        }
    });

    function renderResults(data) {
        // Render Top Glass Metrics
        metricTokens.textContent = data.total_tokens !== undefined ? data.total_tokens : '-';
        const errorsCount = (data.errors || []).length;
        const warningsCount = (data.warnings || []).length;
        const issuesCount = errorsCount + warningsCount;
        
        metricIssues.textContent = issuesCount;
        metricFuncs.textContent = data.total_funcs !== undefined ? data.total_funcs : '-';
        metricVars.textContent = data.total_vars !== undefined ? data.total_vars : '-';

        // Render Tokens List
        tokensBody.innerHTML = '';
        if (data.tokens && data.tokens.length > 0) {
            data.tokens.forEach(t => {
                const tr = document.createElement('tr');
                tr.innerHTML = `
                    <td>${t.line}</td>
                    <td><span class="t-badge t-${t.type}">${t.type}</span></td>
                    <td>${escapeHtml(t.value)}</td>
                `;
                tokensBody.appendChild(tr);
            });
        } else {
            tokensBody.innerHTML = `<tr><td colspan="3" class="placeholder-text">No tokens parsed.</td></tr>`;
        }

        // Render Semantic AI Summary
        if (data.ai_summary) {
            summaryPanel.style.display = 'block';
            aiSummaryText.textContent = data.ai_summary;
            
            // Score handling inside Summary Panel
            const score = data.score || 0;
            setTimeout(() => { scoreBar.style.width = `${score}%`; }, 100);
            scoreText.textContent = `${score}%`;
            if (score > 80) scoreBar.style.background = 'var(--success-color)';
            else if (score > 50) scoreBar.style.background = 'var(--warning-color)';
            else scoreBar.style.background = 'var(--error-color)';
        } else {
            summaryPanel.style.display = 'none';
        }

        // Render Issues List
        issuesList.innerHTML = '';
        const allIssues = [
            ...(data.errors || []).map(e => ({ ...e, severity: 'error' })),
            ...(data.warnings || []).map(w => ({ ...w, severity: 'warning' }))
        ];

        if (allIssues.length === 0) {
            issuesList.innerHTML = `
                <div class="glass-panel issue-card" style="border-left: 4px solid var(--success-color); text-align: center;">
                    <h3 style="margin-bottom: 0.5rem; color: var(--success-color)">All Clear! 🎉</h3>
                    <p style="color: var(--text-secondary)">No bugs or semantic issues found in your code.</p>
                </div>
            `;
        } else {
            allIssues.forEach((issue, idx) => {
                const card = document.createElement('div');
                card.className = 'glass-panel issue-card';
                card.style.borderLeft = `4px solid var(--${issue.severity}-color)`;
                
                const hints = issue.hints || [];
                let aiHTML = '';
                if (issue.severity === 'error') {
                    const uId = `ai-box-${idx}`;
                    const expText = issue.ai_explanation || '';
                    const codeSnip = issue.corrected_snippet ? `<pre><code>${escapeHtml(issue.corrected_snippet)}</code></pre>` : '';
                    
                    aiHTML = `
                        <div class="ai-controls">
                            <button class="ai-btn explain-btn" onclick="toggleBox('${uId}', 'explanation')">Explain with AI 🤖</button>
                            ${hints[0] ? `<button class="ai-btn" onclick="toggleBox('${uId}', 'hint1')">Hint 1</button>` : ''}
                            ${hints[1] ? `<button class="ai-btn" onclick="toggleBox('${uId}', 'hint2')">Hint 2</button>` : ''}
                            ${hints[2] ? `<button class="ai-btn" onclick="toggleBox('${uId}', 'hint3')">Full Fix</button>` : ''}
                        </div>
                        <div id="${uId}" class="ai-content-box" data-exp="${encodeURIComponent(expText)}" 
                             data-snip="${encodeURIComponent(codeSnip)}"
                             data-h1="${encodeURIComponent(hints[0] || '')}"
                             data-h2="${encodeURIComponent(hints[1] || '')}"
                             data-h3="${encodeURIComponent(hints[2] || '')}">
                        </div>
                    `;
                }

                card.innerHTML = `
                    <div class="issue-header">
                        <span class="badge ${issue.severity}">${issue.severity}</span>
                        <span class="line-info">Line ${issue.line}</span>
                    </div>
                    <div class="issue-msg">${escapeHtml(issue.message)}</div>
                    ${aiHTML}
                `;
                issuesList.appendChild(card);
            });
        }

        // Render Console Output
        terminalOutput.innerHTML = '';
        if (data.console_output && data.console_output.length > 0) {
            data.console_output.forEach(line => {
                const div = document.createElement('div');
                div.className = 'terminal-line';
                div.textContent = line;
                terminalOutput.appendChild(div);
            });
        } else if (errorsCount > 0) {
            terminalOutput.innerHTML = `<div class="placeholder-text" style="color: #ef4444; font-family: 'Outfit', sans-serif">Execution halted due to semantic errors.</div>`;
        } else {
            terminalOutput.innerHTML = `<div class="placeholder-text" style="color: rgba(255,255,255,0.4); font-family: 'Outfit', sans-serif">No output produced.</div>`;
        }
        
        // Auto-switch tabs based on context
        if (errorsCount > 0) {
            document.querySelector('[data-tab="tab-semantic"]').click();
        } else if (data.console_output && data.console_output.length > 0) {
            document.querySelector('[data-tab="tab-console"]').click();
        } else {
            document.querySelector('[data-tab="tab-tokens"]').click();
        }
    }
});

// Global toggler
window.toggleBox = function(boxId, type) {
    const box = document.getElementById(boxId);
    if (box.style.display === 'block' && box.dataset.currentType === type) {
        box.style.display = 'none';
        return;
    }

    let content = '';
    if (type === 'explanation') {
        content = `<strong>AI Explanation:</strong><br>${decodeURIComponent(box.dataset.exp)}<br>${decodeURIComponent(box.dataset.snip)}`;
    } else if (type === 'hint1') {
        content = `<strong>Hint 1:</strong><br>${decodeURIComponent(box.dataset.h1)}`;
    } else if (type === 'hint2') {
        content = `<strong>Hint 2:</strong><br>${decodeURIComponent(box.dataset.h2)}`;
    } else if (type === 'hint3') {
        content = `<strong>Full Fix:</strong><br>${decodeURIComponent(box.dataset.h3)}<br>${decodeURIComponent(box.dataset.snip)}`;
    }

    box.innerHTML = content;
    box.style.display = 'block';
    box.dataset.currentType = type;
};

function escapeHtml(unsafe) {
    return unsafe
         .replace(/&/g, "&amp;")
         .replace(/</g, "&lt;")
         .replace(/>/g, "&gt;")
         .replace(/"/g, "&quot;")
         .replace(/'/g, "&#039;");
}
