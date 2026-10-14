"""Generate a Ripes GUI source or the renderer-disabled measurement ELF."""
from pathlib import Path
import argparse, re

root = Path(__file__).resolve().parent
p = argparse.ArgumentParser()
p.add_argument('--mode', choices=['gui', 'cli'], default='gui')
p.add_argument('--input', help='14-character cube input; defaults to the source input')
args = p.parse_args()
core = (root / 'solver_core.s').read_text(encoding='utf-8-sig')
tables = (root / 'tables.s').read_text(encoding='utf-8-sig')
if args.input:
    if not re.fullmatch(r'[1-7]{14}', args.input):
        p.error('Input must contain 14 digits from 1 to 7; target validation checks legality.')
    core, count = re.subn(r'\.asciz "[1-7]{14}"', '.asciz "' + args.input + '"', core, count=1)
    assert count == 1

# The core retains assemble-time guards; remove them before the restricted
# assemblers see the source. GUI keeps the renderer, CLI omits it completely.
lines = []
keep = True
for line in core.splitlines():
    stripped = line.strip()
    if re.match(r'\.equ\s+RENDER\s*,', stripped):
        continue
    if stripped == '.if RENDER':
        keep = args.mode == 'gui'
        continue
    if stripped == '.endif':
        keep = True
        continue
    if keep:
        lines.append(line)

if args.mode == 'gui':
    # This Ripes editor accepts .data for the embedded tables.
    combined = '\n'.join(lines) + '\n' + tables.replace('.section .rodata', '.data')
    output = root / 'solver_led_gui.s'
    output.write_text(combined, encoding='utf-8')
    print('Generated', output.name, '- load as Source file after adding LED Matrix 35x25.')
else:
    from assemble_coordinate import assemble
    output = root / 'solver_cli_generated.s'
    output.write_text('\n'.join(lines) + '\n' + tables, encoding='utf-8')
    assemble(output, root / 'solver_cli.elf')
    print('Generated solver_cli.elf - renderer and LED MMIO references excluded.')
