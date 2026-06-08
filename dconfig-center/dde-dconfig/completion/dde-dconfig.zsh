#compdef dde-dconfig

# SPDX-FileCopyrightText: 2024 Uniontech Software Technology Co.,Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

_dde-dconfig() {
  local ret=1
  local -a commands=(
    'list:list模式，列出可配置的应用Id，配置Id，及子目录'
    'get:get模式，用于查询指定配置项的信息'
    'set:set模式，用于设置配置项的值'
    'reset:reset模式，用于重置配置项的值，此会清除对应的缓存值'
    'watch:watch模式，用于监听配置项的更改'
    'gui:gui模式，用于启动GUI工具，需要安装对应的GUI工具dde-dconfig-editor'
  )

  local -a options=(
    '-a:指定应用Id(appid)'
    '-r:指定配置Id(resource)'
    '-s:指定子目录(subpath)'
    '-k:指定配置项Key值'
  )

  _arguments -C -s -S -n \
    '(* -)'{-v,--version}"[display version information]: :->full" \
    '(- 1 *)'{-h,--help}'[display usage information]: :->full' \
    '1:cmd:->cmds' \
    '*:: :->args' && ret=0

  local prev=${words[CURRENT-1]}
  case "$state" in
  cmds)
    _describe -t commands 'commands' commands
    _describe -t options 'options' options
    ;;
  args)
    case $prev in
    'list' | 'set' | 'get' | 'reset' | 'watch')
      local result=($(dde-dconfig list))
      compadd -a result
      ret=0
      ;;
    '-a')
      local result=($(dde-dconfig list))
      compadd -a result
      ret=0
      ;;
    '-r')
      local a=""
      for ((i = 1; i < CURRENT; i++)); do
        if [[ ${words[i]} == "-a" && $((i+1)) -lt CURRENT ]]; then
          a=${words[i+1]}
          break
        fi
      done
      if [[ -n "$a" ]]; then
        local result=($(dde-dconfig list -a $a 2>/dev/null))
        compadd -a result
      else
        local result=($(dde-dconfig list -r "" 2>/dev/null))
        compadd -a result
      fi
      ret=0
      ;;
    '-s')
      ret=0
      ;;
    '-k')
      local a=""
      local r=""
      for ((i = 1; i < CURRENT; i++)); do
        if [[ ${words[i]} == "-a" && $((i+1)) -lt CURRENT ]]; then
          a=${words[i+1]}
        fi
        if [[ ${words[i]} == "-r" && $((i+1)) -lt CURRENT ]]; then
          r=${words[i+1]}
        fi
      done
      if [[ -n "$a" && -n "$r" ]]; then
        local result=($(dde-dconfig --get -a $a -r $r 2>/dev/null))
        compadd -a result
      fi
      ret=0
      ;;
    *)
      _describe -t options 'options' options
      ret=0
      ;;
    esac
    ;;
  *) ;;
  esac

  return ret
}

_dde-dconfig
