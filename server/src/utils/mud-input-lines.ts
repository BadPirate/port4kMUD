import { MAX_INPUT_LENGTH } from './mud-prompts'

/**
 * Reassembles the lines a player types from the per-keystroke stream the
 * browser sends, exactly as the MUD's own process_input (mud/src/comm.c) does,
 * so the bridge sees the same line the MUD's nanny() will act on.
 *
 * The rules mirrored from that function:
 *  - a backspace (ASCII 8) removes the last character, if any. server.ts
 *    already folds DEL (0x7f) into 8 before forwarding.
 *  - only printable ASCII is kept, so escape sequences from arrow keys and
 *    function keys contribute nothing.
 *  - CR and LF both terminate a line, and a run of them terminates exactly
 *    one: process_input skips past every newline before looking for the next.
 *  - lines are capped at MAX_INPUT_LENGTH - 1 characters.
 *
 * Not mirrored is process_input's doubling of '$' in the line it stores. That
 * happens on the MUD's side of the wire, identically for a password typed by
 * a player and for the same password replayed by the bridge later, so the
 * bridge is correct to record what was typed.
 */
export class MudInputLineAssembler {
  private current = ''

  /** True while skipping the rest of a run of newline characters. */
  private inNewlineRun = false

  /**
   * Feeds raw input. Returns the lines completed by this chunk, in order; a
   * chunk usually completes none (a single keystroke) or one (Enter).
   */
  push(data: string): string[] {
    const lines: string[] = []

    for (const char of data) {
      const code = char.charCodeAt(0)

      if (code === 13 || code === 10) {
        if (!this.inNewlineRun) {
          this.inNewlineRun = true
          lines.push(this.current)
          this.current = ''
        }
        continue
      }

      this.inNewlineRun = false

      if (code === 8) {
        this.current = this.current.slice(0, -1)
        continue
      }

      if (code >= 32 && code <= 126 && this.current.length < MAX_INPUT_LENGTH - 1) {
        this.current += char
      }
    }

    return lines
  }

  /** The partially typed line, for tests and diagnostics. */
  get pending(): string {
    return this.current
  }

  reset(): void {
    this.current = ''
    this.inNewlineRun = false
  }
}
