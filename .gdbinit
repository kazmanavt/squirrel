define dof
  if $argc == 1
    set detach-on-fork $arg0
  end
  show detach-on-fork
end
