from flask import Flask, request, jsonify, render_template
import subprocess
import os
import json
from ai_module import enhance_report_with_ai

app = Flask(__name__)

# Correctly handle OS extension
EXE_NAME = "analyzer.exe" if os.name == "nt" else "./analyzer.exe" 

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/analyze', methods=['POST'])
def analyze():
    data = request.get_json()
    code = data.get('code', '')
    
    # User Requirement: Custom error if executable doesn't exist
    if not os.path.exists(EXE_NAME.replace('./', '')):
        return jsonify({
            "status": "error",
            "score": 0,
            "errors": [{
                "line": 0,
                "type": "SystemError",
                "message": "Compiler not built — run compile.bat first"
            }],
            "warnings": [],
            "ai_summary": "System error preventing analysis."
        })

    try:
        # Run C++ executable, pass code via stdin
        process = subprocess.Popen([EXE_NAME], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        stdout, stderr = process.communicate(input=code)
        
        try:
            report = json.loads(stdout)
        except json.JSONDecodeError:
            # Fallback if C++ output isn't valid JSON (e.g. segfault)
            report = {
                "status": "error",
                "score": 0,
                "errors": [{"line": 0, "type": "InternalError", "message": "Failed to parse compiler output."}],
                "warnings": []
            }
            
        # Call AI for enhanced explanation and hints
        enhanced_report = enhance_report_with_ai(report, code)
        
        return jsonify(enhanced_report)

    except Exception as e:
        return jsonify({
            "status": "error",
            "score": 0,
            "errors": [{
                "line": 0,
                "type": "ExecutionError",
                "message": str(e)
            }],
            "warnings": [],
            "ai_summary": "Fatal execution fault."
        })

if __name__ == '__main__':
    app.run(debug=True, port=5000)
