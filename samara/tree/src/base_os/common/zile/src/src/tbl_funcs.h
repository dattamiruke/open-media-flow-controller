/*	$Id: tbl_funcs.h,v 1.1.1.1 2003/09/19 17:05:55 gregs Exp $	*/

/*
 * Copyright (c) 1997-2001 Sandro Sigala.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Add an entry to this list for declaring a new function.
 * X0 means no key binding, X1 means one key binding, etc.
 *
 * Please remember to keep in sync with the Texinfo documentation
 * `../doc/zile.texi', the manual page `zile.1.in' and the reference card
 * `../etc/refcard.tex'.
 */

X0("auto-fill-mode", auto_fill_mode)
X2("backward-char", backward_char,			"\\C-b", "\\LEFT")
X1("backward-delete-char", backward_delete_char,	"\\BS")
#if 0
X1("backward-kill-sentence", backward_kill_sentence,	"\\C-x\\DEL")
X1("backward-kill-word", backward_kill_word,		"\\M-\\DEL")
X1("backward-paragraph", backward_paragraph,		"\\M-[")
X1("backward-sentence", backward_sentence,		"\\M-a")
X1("backward-word", backward_word,			"\\M-b")
#endif
X1("beginning-of-buffer", beginning_of_buffer,		"\\M-<")
X2("beginning-of-line", beginning_of_line,		"\\C-a", "\\HOME")
X0("c-mode", c_mode)
X0("c++-mode", cpp_mode)
X2("call-last-kbd-macro", call_last_kbd_macro,		"\\C-xe", "\\F12")
#if 0
X1("capitalize-word", capitalize_word,			"\\M-c")
#endif
X0("cd", cd)
X1("copy-region-as-kill", copy_region_as_kill,		"\\M-w")
X1("copy-to-register", copy_to_register,		"\\C-xrs")
X2("delete-char", delete_char,				"\\C-d", "\\DEL")
X1("delete-other-windows", delete_other_windows,	"\\C-x1")
X1("delete-window", delete_window,			"\\C-x0")
X2("describe-function", describe_function,		"\\C-hd", "\\C-hf")
X1("describe-key", describe_key,			"\\C-hk")
X1("describe-variable", describe_variable,		"\\C-hv")
X1("downcase-region", downcase_region,			"\\C-x\\C-l")
#if 0
X1("downcase-word", downcase_word,			"\\M-l")
#endif
X1("end-kbd-macro", end_kbd_macro,			"\\C-x)")
X1("end-of-buffer", end_of_buffer,			"\\M->")
X2("end-of-line", end_of_line,				"\\C-e", "\\END")
X1("enlarge-window", enlarge_window,			"\\C-x^")
X1("exchange-point-and-mark", exchange_point_and_mark,	"\\C-x\\C-x")
X1("execute-extended-command", execute_extended_command,"\\M-x")
X1("find-alternate-file", find_alternate_file,		"\\C-x\\C-v")
X2("find-file", find_file,				"\\C-x\\C-f", "\\F2")
X0("font-lock-mode", font_lock_mode)
X0("font-lock-refresh", font_lock_refresh)
X2("forward-char", forward_char,			"\\C-f", "\\RIGHT")
#if 0
X1("forward-paragraph", forward_paragraph,		"\\M-]")
X1("forward-sentence", forward_sentence,		"\\M-e")
X1("forward-word", forward_word,			"\\M-f")
#endif
X1("goto-line", goto_line,				"\\M-g")
X2("help", help,					"\\C-hh", "\\F1")
X1("help-config-sample", help_config_sample,		"\\C-hs")
X1("help-faq", help_faq,				"\\C-hF")
X1("help-latest-version", help_latest_version,		"\\C-h\\C-d")
X1("help-tutorial", help_tutorial,			"\\C-ht")
X0("insert-buffer", insert_buffer)
X1("insert-file", insert_file,				"\\C-xi")
X1("insert-register", insert_register,			"\\C-xri")
X1("isearch-backward", isearch_backward,		"\\C-r")
X1("isearch-forward", isearch_forward,			"\\C-s")
X1("keyboard-quit", keyboard_quit,			"\\C-g")
X1("kill-buffer", kill_buffer,				"\\C-xk")
X2("kill-line", kill_line,				"\\C-k", "\\F6")
X2("kill-region", kill_region,				"\\C-w", "\\F7")
#if 0
X1("kill-sentence", kill_sentence,			"\\M-k")
X1("kill-word", kill_word,				"\\M-d")
#endif
X1("list-bindings", list_bindings,			"\\C-hlb")
X1("list-buffers", list_buffers,			"\\C-x\\C-b")
X1("list-functions", list_functions,			"\\C-hlf")
X1("list-registers", list_registers,			"\\C-hlr")
X1("list-variables", list_variables,			"\\C-hlv")
X1("mark-whole-buffer", mark_whole_buffer,		"\\C-xh")
#if 0
X1("mark-paragraph", mark_paragraph,			"\\M-h")
X1("mark-word", mark_word,				"\\M-@")
#endif
X1("newline", newline,					"\\RET")
X2("next-line", next_line,				"\\C-n", "\\DOWN")
X1("other-window", other_window,			"\\C-xo")
X1("overwrite-mode", overwrite_mode,			"\\INS")
X2("previous-line", previous_line,			"\\C-p", "\\UP")
X0("query-replace", query_replace)
X1("quoted-insert", quoted_insert,			"\\C-q")
X1("recenter", recenter,				"\\C-l")
X0("replace-string", replace_string)
X2("rotate-minihelp-window", rotate_minihelp_window,	"\\C-h\\C-r", "\\F9")
X2("save-buffer", save_buffer,				"\\C-x\\C-s", "\\F3")
X1("save-buffers-kill-zile", save_buffers_kill_zile,	"\\C-x\\C-c")
X1("save-some-buffers", save_some_buffers,		"\\C-xs")
X2("scroll-down", scroll_down,				"\\M-v", "\\PGUP")
X2("scroll-up", scroll_up,				"\\C-v", "\\PGDN")
X0("search-backward", search_backward)
X0("search-forward", search_forward)
X0("self-insert-command", self_insert_command)
X1("set-fill-column", set_fill_column,			"\\C-xf")
X2("set-mark-command", set_mark_command,		"\\C-@", "\\F5")
X0("set-variable", set_variable)
X1("shell-command", shell_command,			"\\M-!")
X1("shell-command-on-region", shell_command_on_region,	"\\M-|")
X0("shell-script-mode", shell_script_mode)
X1("split-window", split_window,			"\\C-x2")
X1("start-kbd-macro", start_kbd_macro,			"\\C-x(")
X2("suspend-zile", suspend_zile,			"\\C-x\\C-z", "\\C-z")
X1("switch-to-buffer", switch_to_buffer,		"\\C-xb")
X1("switch-to-correlated-buffer", switch_to_correlated_buffer, "\\F11")
X0("tabify", tabify)
X1("tab-to-tab-stop", tab_to_tab_stop,			"\\TAB")
X0("text-mode", text_mode)
X2("toggle-minihelp-window", toggle_minihelp_window,	"\\C-h\\C-h", "\\F10")
X1("toggle-read-only", toggle_read_only,		"\\C-x\\C-q")
#if 0
X1("transpose-chars", transpose_chars,			"\\C-t")
X1("transpose-lines", transpose_lines,			"\\C-x\\C-t")
X1("transpose-sexps", transpose_sexps,			"\\C-\\M-t")
X1("transpose-words", transpose_words,			"\\M-t")
#endif
X3("undo", undo,					"\\C-xu", "\\C-_", "\\F4")
X1("universal-argument", universal_argument,		"\\C-u")
X0("untabify", untabify)
X1("upcase-region", upcase_region,			"\\C-x\\C-u")
#if 0
X1("upcase-word", upcase_word,				"\\M-u")
#endif
X1("write-file", write_file,				"\\C-x\\C-w")
X2("yank", yank,					"\\C-y", "\\F8")
X0("zile-version", zile_version)
