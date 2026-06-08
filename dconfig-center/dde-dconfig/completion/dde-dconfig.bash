#!/bin/bash

# SPDX-FileCopyrightText: 2024 Uniontech Software Technology Co.,Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

_dde_dconfig() {
	local cur prev words cword
	_init_completion || return

	local opts=("-a -r -s -k")

	case $prev in
	'dde-dconfig')
		COMPREPLY=($(compgen -W 'help list set get reset gui watch' -- $cur))
		return
		;;
	'list' | 'set' | 'reset' | 'watch')
		local result=($(dde-dconfig list))
		COMPREPLY=($(compgen -W "${result[*]}" -- $cur))
		return
		;;
	'get')
		if [[ $cur == [-]* ]]; then
			COMPREPLY=($(compgen -W "${opts}" -- ${cur}))
			return
		fi
		local result=($(dde-dconfig list))
		COMPREPLY=($(compgen -W "${result[*]}" -- $cur))
		return
		;;
	'-a')
		local result=($(dde-dconfig list))
		COMPREPLY=($(compgen -W "${result[*]}" -- $cur))
		return
		;;
	'-r')
		local a=""
		for ((i = 0; i < ${#words[@]}; i++)); do
			if [[ ${words[i]} == "-a" && $((i+1)) -lt ${#words[@]} ]]; then
				a=${words[i+1]}
				break
			fi
		done
		if [[ -n "$a" ]]; then
			local result=($(dde-dconfig list -a $a 2>/dev/null))
			COMPREPLY=($(compgen -W "${result[*]}" -- $cur))
		else
			local result=($(dde-dconfig list -r "" 2>/dev/null))
			COMPREPLY=($(compgen -W "${result[*]}" -- $cur))
		fi
		return
		;;
	'-s')
		return
		;;
	'-k')
		local a=""
		local r=""
		for ((i = 0; i < ${#words[@]}; i++)); do
			if [[ ${words[i]} == "-a" && $((i+1)) -lt ${#words[@]} ]]; then
				a=${words[i+1]}
			fi
			if [[ ${words[i]} == "-r" && $((i+1)) -lt ${#words[@]} ]]; then
				r=${words[i+1]}
			fi
		done
		if [[ -n "$a" && -n "$r" ]]; then
			local result=($(dde-dconfig --get -a $a -r $r 2>/dev/null))
			COMPREPLY=($(compgen -W "${result[*]}" -- $cur))
		fi
		return
		;;
	esac

	if [[ $cur == [-]* ]]; then
		COMPREPLY=($(compgen -W "${opts}" -- ${cur}))
	fi

	if ((${#COMPREPLY[@]} != 0)); then
		local -A onlyonce=([-a]=1 [-r]=1 [-s]=1 [-k]=1)
		local j
		for i in "${words[@]}"; do
			[[ $i && -v onlyonce["$i"] ]] || continue
			for j in "${!COMPREPLY[@]}"; do
				[[ ${COMPREPLY[j]} == "$i" ]] && unset 'COMPREPLY[j]'
			done
		done
	fi

} &&
	complete -F _dde_dconfig dde-dconfig
