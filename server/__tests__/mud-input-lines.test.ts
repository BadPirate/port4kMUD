import { MudInputLineAssembler } from '../src/utils/mud-input-lines'
import { MAX_INPUT_LENGTH } from '../src/utils/mud-prompts'

/** Types a string one keystroke at a time, as the browser sends it. */
function type(assembler: MudInputLineAssembler, text: string): string[] {
  const lines: string[] = []
  for (const char of text) lines.push(...assembler.push(char))
  return lines
}

describe('MudInputLineAssembler', () => {
  it('emits a line when Enter is pressed', () => {
    const assembler = new MudInputLineAssembler()

    expect(type(assembler, 'Zephyrtest\r')).toEqual(['Zephyrtest'])
  })

  it('emits nothing until the line is terminated', () => {
    const assembler = new MudInputLineAssembler()

    expect(type(assembler, 'Zephyr')).toEqual([])
    expect(assembler.pending).toBe('Zephyr')
  })

  it.each([
    ['carriage return', '\r'],
    ['line feed', '\n'],
  ])('treats %s as a terminator', (_label, terminator) => {
    const assembler = new MudInputLineAssembler()

    expect(type(assembler, `Bob${terminator}`)).toEqual(['Bob'])
  })

  it('treats a run of newlines as one terminator', () => {
    // process_input skips past every newline before looking for the next
    // line, so CRLF is one Enter and not two.
    const assembler = new MudInputLineAssembler()

    expect(type(assembler, 'Bob\r\n')).toEqual(['Bob'])
  })

  it('emits an empty line for Enter on its own', () => {
    const assembler = new MudInputLineAssembler()

    expect(type(assembler, '\r\n')).toEqual([''])
  })

  it('emits two lines for two separated Enters', () => {
    const assembler = new MudInputLineAssembler()

    expect(type(assembler, 'a\r\nb\r\n')).toEqual(['a', 'b'])
  })

  it('removes the last character on backspace', () => {
    const assembler = new MudInputLineAssembler()

    expect(type(assembler, 'Bobx\bby\b\r')).toEqual(['Bobb'])
  })

  it('ignores backspace on an empty line', () => {
    const assembler = new MudInputLineAssembler()

    expect(type(assembler, '\b\b\bBob\r')).toEqual(['Bob'])
  })

  it('drops non-printable input such as arrow keys', () => {
    // An escape sequence reaches the MUD as its printable characters only,
    // because process_input keeps just isascii && isprint bytes.
    const assembler = new MudInputLineAssembler()

    expect(type(assembler, 'Bo\u001b[Ab\r')).toEqual(['Bo[Ab'])
  })

  it('handles a whole pasted line arriving in one chunk', () => {
    const assembler = new MudInputLineAssembler()

    expect(assembler.push('Zephyrtest\r\nsekrit1\r\n')).toEqual(['Zephyrtest', 'sekrit1'])
  })

  it('caps a line at the length the MUD accepts', () => {
    const assembler = new MudInputLineAssembler()
    const [line] = type(assembler, `${'x'.repeat(MAX_INPUT_LENGTH + 50)}\r`)

    expect(line).toHaveLength(MAX_INPUT_LENGTH - 1)
  })

  it('forgets a partial line when reset', () => {
    const assembler = new MudInputLineAssembler()
    type(assembler, 'Zephyr')
    assembler.reset()

    expect(assembler.pending).toBe('')
    expect(type(assembler, 'Bob\r')).toEqual(['Bob'])
  })
})
