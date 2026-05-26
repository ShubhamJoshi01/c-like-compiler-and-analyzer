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
    const astTreeContainer = document.getElementById('ast-tree-container');

    // Metrics elements
    const metricTokens = document.getElementById('metric-tokens');
    const metricIssues = document.getElementById('metric-issues');
    const metricFuncs = document.getElementById('metric-funcs');
    const metricVars = document.getElementById('metric-vars');
    const scoreBar = document.getElementById('score-bar');
    const scoreText = document.getElementById('score-text');

    // CodeMirror Line Highlights Management
    let highlightedLineHandles = [];

    // Code templates mapping
    const templates = {
        basic: `int add(int a, int b) {
    return a + b;
}

int main() {
    int x = 5;
    int y = 10;
    int result = add(x, y);
    print(result);
    
    // Simple unused variable to show semantic linter warning
    int unusedVar = 99; 
    
    return 0;
}`,
        logical: `int checkLogic(int a, int b) {
    // Tests relational comparisons and logical negation (!)
    if (a > 0 && !b) {
        return 1;
    }
    return 0;
}

int main() {
    int x = 10;
    int y = 0;
    
    // Unused variable to trigger a static quality warning card!
    int unusedVar = 99; 

    // Tests logical AND (&&) combining relations and function calls
    if (x > 5 && checkLogic(x, y)) {
        print('t');
        print('r');
        print('u');
        print('e');
    } else {
        print('f');
    }
    
    return 0;
}`,
        recursive: `int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int num = 5;
    int result = factorial(num);
    
    print('f');
    print('a');
    print('c');
    print('t');
    print(':');
    print(result); // In this compiler print takes ints/chars, prints value
    
    return 0;
}`,
        mismatch: `int main() {
    int x = 10;
    char c = 'a';
    
    // Semantic Error: Type mismatch in assignment
    x = c; 
    
    // Semantic Error: Variable used before assignment
    int uninit;
    int y = uninit + 5;
    
    return 0;
}`
    };

    // Template selection change handler
    const templateSelect = document.getElementById('template-select');
    if (templateSelect) {
        templateSelect.addEventListener('change', () => {
            const selectedVal = templateSelect.value;
            if (templates[selectedVal]) {
                codeEditor.setValue(templates[selectedVal]);
                clearHighlightedLines();
                // Dynamic Auto-Analysis on selection!
                setTimeout(() => { analyzeBtn.click(); }, 150);
            }
        });
    }

    function clearHighlightedLines() {
        highlightedLineHandles.forEach(h => {
            codeEditor.removeLineClass(h, 'background', 'cm-error-line');
            codeEditor.removeLineClass(h, 'background', 'cm-warning-line');
        });
        highlightedLineHandles = [];
    }

    // Clear highlights as soon as the user edits code
    codeEditor.on('change', clearHighlightedLines);

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
        clearHighlightedLines();

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

    // Helper to recursively build collapsible AST tree
    function createAstNodeHtml(node) {
        if (!node) return null;
        
        const li = document.createElement('li');
        const nodeDiv = document.createElement('div');
        nodeDiv.className = 'ast-node';
        
        let hasChildren = false;
        const children = [];
        
        // Extract children based on node type
        if (node.type === 'Program' || node.type === 'Block') {
            if (node.statements && node.statements.length > 0) {
                hasChildren = true;
                node.statements.forEach(s => children.push(s));
            }
        } else if (node.type === 'FunctionDecl') {
            if (node.block) {
                hasChildren = true;
                children.push(node.block);
            }
        } else if (node.type === 'VarDecl') {
            if (node.initializer) {
                hasChildren = true;
                children.push(node.initializer);
            }
        } else if (node.type === 'Assign') {
            if (node.expr) {
                hasChildren = true;
                children.push(node.expr);
            }
        } else if (node.type === 'If') {
            if (node.condition) children.push({ ...node.condition, _meta: 'Condition' });
            if (node.true_block) children.push({ ...node.true_block, _meta: 'Then' });
            if (node.false_block) children.push({ ...node.false_block, _meta: 'Else' });
            if (children.length > 0) hasChildren = true;
        } else if (node.type === 'While') {
            if (node.condition) children.push({ ...node.condition, _meta: 'Condition' });
            if (node.block) children.push({ ...node.block, _meta: 'Body' });
            if (children.length > 0) hasChildren = true;
        } else if (node.type === 'For') {
            if (node.init) children.push({ ...node.init, _meta: 'Init' });
            if (node.condition) children.push({ ...node.condition, _meta: 'Condition' });
            if (node.update) children.push({ ...node.update, _meta: 'Update' });
            if (node.block) children.push({ ...node.block, _meta: 'Body' });
            if (children.length > 0) hasChildren = true;
        } else if (node.type === 'Return' || node.type === 'Print') {
            if (node.expr) {
                hasChildren = true;
                children.push(node.expr);
            }
        } else if (node.type === 'BinaryOp') {
            if (node.left) children.push(node.left);
            if (node.right) children.push(node.right);
            if (children.length > 0) hasChildren = true;
        } else if (node.type === 'UnaryOp') {
            if (node.expr) {
                hasChildren = true;
                children.push(node.expr);
            }
        } else if (node.type === 'FunctionCall') {
            if (node.args && node.args.length > 0) {
                hasChildren = true;
                node.args.forEach(a => children.push(a));
            }
        }
        
        // Add collapse toggle
        if (hasChildren) {
            const toggle = document.createElement('span');
            toggle.className = 'ast-node-toggle';
            toggle.textContent = '▼';
            toggle.addEventListener('click', (e) => {
                e.stopPropagation();
                nodeDiv.classList.toggle('ast-collapsed');
                toggle.textContent = nodeDiv.classList.contains('ast-collapsed') ? '▶' : '▼';
            });
            nodeDiv.appendChild(toggle);
        } else {
            const spacer = document.createElement('span');
            spacer.style.width = '14px';
            nodeDiv.appendChild(spacer);
        }
        
        // Render Meta Prefix if exists (e.g. "[Condition]", "[Then]")
        if (node._meta) {
            const meta = document.createElement('span');
            meta.className = 'ast-node-meta';
            meta.textContent = `[${node._meta}]`;
            nodeDiv.appendChild(meta);
        }
        
        // Badge Type
        const typeBadge = document.createElement('span');
        typeBadge.className = 'ast-node-type';
        typeBadge.textContent = node.type;
        nodeDiv.appendChild(typeBadge);
        
        // Display values based on node type
        if (node.type === 'Literal') {
            const valSpan = document.createElement('span');
            valSpan.className = 'ast-node-val';
            valSpan.textContent = node.val_type === 'char' ? `'${node.value}'` : node.value;
            
            const metaSpan = document.createElement('span');
            metaSpan.className = 'ast-node-meta';
            metaSpan.textContent = `(${node.val_type})`;
            
            nodeDiv.appendChild(valSpan);
            nodeDiv.appendChild(metaSpan);
        } else if (node.type === 'Identifier') {
            const nameSpan = document.createElement('span');
            nameSpan.className = 'ast-node-name';
            nameSpan.textContent = node.name;
            nodeDiv.appendChild(nameSpan);
        } else if (node.type === 'BinaryOp' || node.type === 'UnaryOp') {
            const opSpan = document.createElement('span');
            opSpan.className = 'ast-node-op';
            opSpan.textContent = node.op;
            nodeDiv.appendChild(opSpan);
        } else if (node.type === 'VarDecl') {
            const nameSpan = document.createElement('span');
            nameSpan.className = 'ast-node-name';
            nameSpan.textContent = node.name;
            
            const metaSpan = document.createElement('span');
            metaSpan.className = 'ast-node-meta';
            metaSpan.textContent = `(${node.val_type})`;
            
            nodeDiv.appendChild(nameSpan);
            nodeDiv.appendChild(metaSpan);
        } else if (node.type === 'Assign') {
            const nameSpan = document.createElement('span');
            nameSpan.className = 'ast-node-name';
            nameSpan.textContent = node.name;
            nodeDiv.appendChild(nameSpan);
        } else if (node.type === 'FunctionDecl') {
            const nameSpan = document.createElement('span');
            nameSpan.className = 'ast-node-name';
            nameSpan.textContent = `${node.name}()`;
            
            const metaSpan = document.createElement('span');
            metaSpan.className = 'ast-node-meta';
            metaSpan.textContent = `returns ${node.return_type}`;
            
            nodeDiv.appendChild(nameSpan);
            nodeDiv.appendChild(metaSpan);
        } else if (node.type === 'FunctionCall') {
            const nameSpan = document.createElement('span');
            nameSpan.className = 'ast-node-name';
            nameSpan.textContent = `${node.name}()`;
            nodeDiv.appendChild(nameSpan);
        }
        
        // Line number
        if (node.line) {
            const lineSpan = document.createElement('span');
            lineSpan.className = 'ast-node-meta';
            lineSpan.style.opacity = '0.5';
            lineSpan.textContent = `L${node.line}`;
            nodeDiv.appendChild(lineSpan);
        }
        
        li.appendChild(nodeDiv);
        
        // Append child nodes
        if (hasChildren) {
            const childrenUl = document.createElement('ul');
            children.forEach(child => {
                const childHtml = createAstNodeHtml(child);
                if (childHtml) childrenUl.appendChild(childHtml);
            });
            li.appendChild(childrenUl);
        }
        
        return li;
    }

    function renderResults(data) {
        // Clear previous highlights
        clearHighlightedLines();

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
                
                // Highlight lines in CodeMirror (lines are 1-indexed in report, 0-indexed in CM)
                if (issue.line > 0) {
                    const lineHandle = codeEditor.addLineClass(
                        issue.line - 1, 
                        'background', 
                        issue.severity === 'error' ? 'cm-error-line' : 'cm-warning-line'
                    );
                    highlightedLineHandles.push(lineHandle);
                }

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

        // Render Collapsible Visual AST Tree
        astTreeContainer.innerHTML = '';
        if (data.ast) {
            const rootUl = document.createElement('ul');
            const rootLi = createAstNodeHtml(data.ast);
            if (rootLi) {
                rootUl.appendChild(rootLi);
                astTreeContainer.appendChild(rootUl);
            } else {
                astTreeContainer.innerHTML = `<div class="placeholder-text">AST could not be visualized.</div>`;
            }
        } else {
            astTreeContainer.innerHTML = `<div class="placeholder-text">No AST parsed. Enter valid code to build AST.</div>`;
        }

        // Update Pipeline Flow Steps dynamically
        const lexerBadge = document.getElementById('status-lexer');
        const parserBadge = document.getElementById('status-parser');
        const semanticBadge = document.getElementById('status-semantic');
        const interpreterBadge = document.getElementById('status-interpreter');

        if (lexerBadge && parserBadge && semanticBadge && interpreterBadge) {
            // Lexer step
            if (data.total_tokens !== undefined) {
                lexerBadge.className = 'flow-badge status-success';
                lexerBadge.innerHTML = `✔️ Scanned ${data.total_tokens} tokens successfully.`;
            } else {
                lexerBadge.className = 'flow-badge status-pending';
                lexerBadge.innerHTML = 'Pending Analysis...';
            }

            // Parser step
            const syntaxError = (data.errors || []).find(e => e.type === 'SyntaxError');
            if (syntaxError) {
                parserBadge.className = 'flow-badge status-error';
                parserBadge.innerHTML = `❌ Failed: Syntax error on line ${syntaxError.line || 1}`;
            } else if (data.ast) {
                parserBadge.className = 'flow-badge status-success';
                parserBadge.innerHTML = '✔️ Abstract Syntax Tree built successfully.';
            } else {
                parserBadge.className = 'flow-badge status-pending';
                parserBadge.innerHTML = 'Pending Analysis...';
            }

            // Semantic step
            const semanticErrors = (data.errors || []).filter(e => e.type !== 'SyntaxError');
            if (data.status === 'error' && syntaxError) {
                semanticBadge.className = 'flow-badge status-pending';
                semanticBadge.innerHTML = 'Halted: Waiting for syntax resolution...';
            } else if (semanticErrors.length > 0) {
                semanticBadge.className = 'flow-badge status-error';
                semanticBadge.innerHTML = `❌ Failed: Static checks failed with ${semanticErrors.length} errors.`;
            } else if (warningsCount > 0) {
                semanticBadge.className = 'flow-badge status-warning';
                semanticBadge.innerHTML = `⚠️ Passed with ${warningsCount} warning(s) (Score: ${data.score}%).`;
            } else if (data.score !== undefined) {
                semanticBadge.className = 'flow-badge status-success';
                semanticBadge.innerHTML = `✔️ Clean build! Passed static checks (Score: ${data.score}%).`;
            } else {
                semanticBadge.className = 'flow-badge status-pending';
                semanticBadge.innerHTML = 'Pending Analysis...';
            }

            // Interpreter step
            if (data.status === 'error' && syntaxError) {
                interpreterBadge.className = 'flow-badge status-pending';
                interpreterBadge.innerHTML = 'Halted: Waiting for syntax resolution...';
            } else if (semanticErrors.length > 0) {
                interpreterBadge.className = 'flow-badge status-warning';
                interpreterBadge.innerHTML = '⚠️ Halted: Prevented execution due to semantic errors.';
            } else if (data.console_output !== undefined) {
                interpreterBadge.className = 'flow-badge status-success';
                interpreterBadge.innerHTML = `✔️ Run complete: Printed ${data.console_output.length} console lines.`;
            } else {
                interpreterBadge.className = 'flow-badge status-pending';
                interpreterBadge.innerHTML = 'Pending Analysis...';
            }
        }
        
        // Auto-switch tabs based on context
        if (errorsCount > 0) {
            document.querySelector('[data-tab="tab-semantic"]').click();
        } else if (data.console_output && data.console_output.length > 0) {
            document.querySelector('[data-tab="tab-console"]').click();
        } else {
            document.querySelector('[data-tab="tab-ast"]').click();
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
