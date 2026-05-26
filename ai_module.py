import os
from openai import OpenAI
import json

# Manually parse .env file if it exists, to avoid external dependency issues
if os.path.exists(".env"):
    with open(".env") as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#") and "=" in line:
                key, val = line.split("=", 1)
                os.environ[key.strip()] = val.strip().strip('"').strip("'")

# Determine if a valid OpenAI key is set
api_key = os.environ.get("OPENAI_API_KEY", "")
has_valid_key = api_key != "" and api_key != "YOUR_API_KEY"

client = None
if has_valid_key:
    client = OpenAI(api_key=api_key)

def get_offline_explanation(error_msg, error_type, line_no):
    """
    Fallback structural logic explaining semantic errors offline/without API key.
    """
    msg = error_msg.lower()
    explanation = "The compiler detected a static semantic error."
    snippet = "// Fix your statement structure"
    hints = [
        "Review C-like type check rules.",
        "Check your syntax declaration or type assignment.",
        "Rewrite or remove the statement."
    ]

    if "unused" in msg:
        explanation = "The variable was declared but never referenced in this scope. In strict compilers, unused variables are flagged as warnings to prevent waste of stack memory."
        snippet = "// You can safely remove the unused declaration"
        hints = [
            "Use the variable in an operation or statement.",
            "Remove the variable definition if it is no longer needed.",
            "Make sure you didn't misspell the variable elsewhere."
        ]
    elif "undeclared" in msg:
        explanation = "You are trying to access or assign a variable that has not been declared with a type (like 'int' or 'char') in the current scope."
        snippet = "int " + msg.split("'")[1] + " = 0; // Declare it first" if "'" in msg else "int x = 0;"
        hints = [
            "Declare the variable at the top of the block or before using it.",
            "Ensure the variable name is spelled correctly (case-sensitive).",
            "If it is a function, ensure the function declaration exists."
        ]
    elif "mismatch" in msg:
        explanation = "Type mismatch! You are trying to assign or operate on incompatible types (e.g. assigning an 'int' expression to a 'char' variable, or vice-versa)."
        snippet = "// Ensure both sides of the assignment match in type"
        hints = [
            "Cast the expression or change the variable definition type.",
            "Check that binary operator types are compatible.",
            "Ensure you are not assigning a void function call output."
        ]
    elif "already declared" in msg:
        explanation = "Re-declaration error! A variable or function with this name has already been defined in the current scope."
        snippet = "// Choose a unique name"
        hints = [
            "Change the variable name to be unique.",
            "Remove the duplicate declaration.",
            "Reuse the existing variable rather than redeclaring it."
        ]
    elif "before assignment" in msg:
        explanation = "You are trying to read a variable before it has been initialized with a value, which results in undefined garbage data."
        snippet = "// Initialize variable: int x = 0;"
        hints = [
            "Assign a default value (e.g., = 0) when declaring the variable.",
            "Ensure the assignment statement executes before the read statement.",
            "Check your control flow branches."
        ]

    return {
        "explanation": explanation,
        "corrected_snippet": snippet,
        "hints": hints
    }

def enhance_report_with_ai(report, source_code):
    """
    Takes the raw JSON report from C++ and augments it using OpenAI GPT-4o,
    or falls back to a robust built-in rule-based linter if no API Key is set.
    """
    errors = report.get('errors', [])
    warnings = report.get('warnings', [])
    
    # 1. Generate Summary Card
    has_issues = len(errors) > 0 or len(warnings) > 0
    if not has_issues:
        report['ai_summary'] = "Your code looks great! No semantic errors or warnings detected."
    else:
        if has_valid_key:
            try:
                summary_prompt = f"Analyze the following C-like code and its errors/warnings. Provide a concise 3-line summary of the overall code health and main issues.\nCode:\n{source_code}\nIssues:\nErrors: {len(errors)}, Warnings: {len(warnings)}"
                response = client.chat.completions.create(
                    model="gpt-4o",
                    messages=[{"role": "user", "content": summary_prompt}],
                    max_tokens=150
                )
                report['ai_summary'] = response.choices[0].message.content.strip()
            except Exception as e:
                report['ai_summary'] = f"(AI Summary Offline) Found {len(errors)} errors and {len(warnings)} warnings."
        else:
            report['ai_summary'] = f"🔒 (Offline Advisor Active) Found {len(errors)} errors and {len(warnings)} warnings. To unlock deep GPT-4o analysis, add a valid OPENAI_API_KEY to your '.env' file in the project folder."

    # 2. Generate explanations and progressive hints for each error
    for error in errors + warnings:
        err_msg = error.get('message', '')
        err_type = error.get('type', '')
        err_line = error.get('line', 0)
        
        if has_valid_key:
            try:
                error_prompt = f"""
                The following C-like code has an issue on line {err_line}: "{err_msg}".
                Code:
                {source_code}
                
                Provide a JSON response with exactly these keys:
                "explanation": A plain English explanation of why the error happened.
                "corrected_snippet": A corrected snippet for the specific lines involved.
                "hint1": A vague nudge on how to fix it.
                "hint2": A more specific direction.
                "hint3": The full fix instructions.
                """
                
                response = client.chat.completions.create(
                    model="gpt-4o",
                    messages=[{"role": "user", "content": error_prompt}],
                    response_format={ "type": "json_object" }
                )
                
                ai_data = json.loads(response.choices[0].message.content)
                
                error['ai_explanation'] = ai_data.get('explanation', "Explanation missing.")
                error['corrected_snippet'] = ai_data.get('corrected_snippet', "")
                error['hints'] = [
                    ai_data.get('hint1', 'Check your syntax.'),
                    ai_data.get('hint2', 'Look closely at the types.'),
                    ai_data.get('hint3', 'Replace it with proper code.')
                ]
            except Exception as e:
                # Fallback to local rule-based engine if call fails
                offline = get_offline_explanation(err_msg, err_type, err_line)
                error['ai_explanation'] = f"{offline['explanation']} (API call failed: {str(e)})"
                error['corrected_snippet'] = offline['corrected_snippet']
                error['hints'] = offline['hints']
        else:
            # Full local rule linter fallback when key is not provided (avoids calling OpenAI and throwing 401)
            offline = get_offline_explanation(err_msg, err_type, err_line)
            error['ai_explanation'] = offline['explanation']
            error['corrected_snippet'] = offline['corrected_snippet']
            error['hints'] = offline['hints']

    return report
