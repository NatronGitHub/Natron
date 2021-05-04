```
cp ../html/_prefs.html prefs-head.txt
cp ../html/_prefs.html prefs-foot.txt
cp ../html/plugins/fr.inria.built-in.Dot.html plugin-head.txt
cp ../html/plugins/fr.inria.built-in.Dot.html plugin-foot.txt
cp ../html/_groupChannel.html groupitem-head.txt
cp ../html/_groupChannel.html groupitem-foot.txt
cp ../html/_group.html group-head.txt
cp ../html/_group.html group-foot.txt

sed -e "s/Natron 2.4.1 documentation/NATRON_DOCUMENTATION/g" -e "s/Dot/%1/g" -e "s/Channel/%1/g" -i .bak *txt

sed -e 's/<link rel="next".*//g' -e 's/<link rel="prev".*//g' -i .bak plugin*txt
```

replacements:

plugin head:

- %1 -> Dot (plugin label)
- %2 -> <li><a href="../_group.html?id=GROUP">GROUP nodes</a> &raquo;</li>

group head:

- %1 -> Channel (group label)

```
for f in plugin-head plugin-foot prefs-head prefs-foot groupitem-head groupitem-foot group-head group-foot; do
   sed -e 's/^[ \t]*//g' -e 's/[ \t]*$//g' -e '/^$/d'  -e ':a;N;$!ba;s/\\n//g' $f.txt | \
   sed -e 's/"/\\"/g' -e 's/^/"/' -e 's/$/\\n"/' > $f.c
done
```