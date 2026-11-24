# Creates ProjectFileFormat-2.5.ntp, the text project saved by Natron 2.5.0 that Tests/ProjectFileFormat_Test.cpp
# loads (built-in nodes only). Run it with the official Natron 2.5.0 release, from this directory:
#   NatronRenderer --no-settings -t ProjectFileFormat-2.5.py
import os, sys
a = app1
g = a.createNode('fr.inria.built-in.Group')
g.setScriptName('Group1')
g.setLabel('Group <&> \xe9')
d = a.createNode('fr.inria.built-in.Dot')
d.setScriptName('Dot1')
d.connectInput(0, g)
b = a.createNode('fr.inria.built-in.BackDrop')
b.setScriptName('BackDrop1')
b.setPosition(-1234.5, 678.25)
p = g.createDoubleParam('myDouble', 'My double')
p.setValueAtTime(0.1, 1)
p.setValueAtTime(-1e300, 2)
p.setValueAtTime(3.141592653589793, 30)
i = g.createIntParam('myInt', 'My int')
i.setValue(-2147483647)
s = g.createStringParam('myString', 'My string')
s.setValue('line 1\nline 2 <tag> & "quotes" €')
c = g.createColorParam('myColor', 'My color', True)
c.setValue(0.25, 0)
c.setValue(1e-30, 3)
e = g.createBooleanParam('myBool', 'My bool')
e.setValue(True)
c.setExpression('thisNode.myDouble.get() * 2', False, 1)
g.refreshUserParamsGUI()
a.getProjectParam('frameRange').set(-5, 123)
print("saved:", a.saveProjectAs(os.path.join(os.getcwd(), 'ProjectFileFormat-2.5.ntp')))
# quit without the clean-up: NatronRenderer 2.5 may crash on exit
sys.stdout.flush()
os._exit(0)
