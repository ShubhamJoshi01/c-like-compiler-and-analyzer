import os
from openai import OpenAI
import json

# Initialize OpenAI Client with placeholder key
client = OpenAI(api_key=os.environ.get("OPENAI_API_KEY", "YOUR_API_KEY"))

def enhance_report_with_ai(report, source_code):
    """
    Takes the raw JSON report from C++ and augments it using OpenAI GPT-4o.
    """
    errors = report.get('errors', [])
    warnings = report.get('warnings', [])
    
    # 1. Generate AI Summary Card for the entire codebase
    has_issues = len(errors) > 0 or len(warnings) > 0
    if not has_issues:
        report['ai_summary'] = "Your code looks great! No semantic errors or warnings detected."
    else:
        try:
            summary_prompt = f"Analyze the following C-like code and its errors/warnings. Provide a concise 3-line summary of the overall code health and main issues.\nCode:\n{source_code}\nIssues:\nErrors: {len(errors)}, Warnings: {len(warnings)}"
            response = client.chat.completions.create(
                model="gpt-4o",
                messages=[{"role": "user", "content": summary_prompt}],
                max_tokens=150
            )
            report['ai_summary'] = response.choices[0].message.content.strip()
        except Exception as e:
            report['ai_summary'] = f"(AI Summary Unavailable) Found {len(errors)} errors and {len(warnings)} warnings."

    # 2. Generate explanations and progressive hints for each error
    for error in errors:
        try:
            err_line = error['line']
            err_msg = error['message']
            
            error_prompt = f"""
            The following C-like code has an error on line {err_line}: "{err_msg}".
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
            # Fallback if OpenAI fails (e.g. invalid API key)
            error['ai_explanation'] = f"AI could not explain this due to an error: {str(e)} (Ensure OpenAI API Key is valid)"
            error['corrected_snippet'] = "// Fix not available"
            error['hints'] = ["Check the error message carefully.", "Review standard rules for this error.", "Rewrite the statement."]

    return report
