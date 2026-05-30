import json
from pathlib import Path
from datetime import datetime, timezone
from graphify.detect import save_manifest

def main():
    # 1. Save manifest for --update
    detect = json.loads(Path('graphify-out/.graphify_detect.json').read_text(encoding="utf-8"))
    save_manifest(detect['files'])

    # 2. Update cumulative cost tracker
    extract = json.loads(Path('graphify-out/.graphify_extract.json').read_text(encoding="utf-8"))
    input_tok = extract.get('input_tokens', 0)
    output_tok = extract.get('output_tokens', 0)

    cost_path = Path('graphify-out/cost.json')
    if cost_path.exists():
        cost = json.loads(cost_path.read_text(encoding="utf-8"))
    else:
        cost = {'runs': [], 'total_input_tokens': 0, 'total_output_tokens': 0}

    # Record this run
    cost['runs'].append({
        'timestamp': datetime.now(timezone.utc).isoformat(),
        'input_tokens': input_tok,
        'output_tokens': output_tok
    })
    cost['total_input_tokens'] += input_tok
    cost['total_output_tokens'] += output_tok

    cost_path.write_text(json.dumps(cost, indent=2, ensure_ascii=False), encoding="utf-8")

    # Clean up scripts
    for s in ['run_graphify.ps1', 'detect.ps1', 'check_cache.ps1', 'detect.py', 'extract_ast.py', 'merge_and_build.py', 'label_communities.py', 'generate_html.py', 'benchmark.py']:
        try:
            Path(s).unlink(missing_ok=True)
        except Exception:
            pass
            
    # Clean up chunk files in graphify-out
    for chunk_file in Path('graphify-out').glob('.graphify_chunk_*.json'):
        try:
            chunk_file.unlink(missing_ok=True)
        except Exception:
            pass
            
    # Clean up temp steps in graphify-out
    for temp_file in Path('graphify-out').glob('.graphify_step_*.py'):
        try:
            temp_file.unlink(missing_ok=True)
        except Exception:
            pass

    print("Cleanup and final manifest saving complete!")

if __name__ == '__main__':
    main()
