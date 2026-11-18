# Recon — puncte de integrare pentru panoul "AI Assistant"

Verificat în arborele curent (`feat/ai-chat-panel`, bază `3763d80`). Fiecare
afirmație are `fișier:linie`.

## 1. `ScriptEditor` — modelul de panou

- `Gui/ScriptEditor.h:46-47` — `class ScriptEditor : public QWidget, public PanelWidget`.
  Moștenire dublă: `QWidget` pentru UI, `PanelWidget` pentru integrarea în sistemul
  de panouri.
- `Gui/ScriptEditor.h:49-51` — `Q_OBJECT` e încadrat de
  `GCC_DIAG_SUGGEST_OVERRIDE_OFF` / `_ON`.
- `Gui/ScriptEditor.h:56` — constructor `ScriptEditor(Gui* gui)`, un singur argument.
- `Gui/ScriptEditor.h:122` — PIMPL: `std::unique_ptr<ScriptEditorPrivate> _imp;`
  (**`std::unique_ptr`**, nu `boost::scoped_ptr` — asta e convenția în `Gui/`).
- `Gui/ScriptEditor.h:115-120` — suprascrie evenimentele de focus/mouse/tastatură ca
  `OVERRIDE FINAL`: `focusInEvent`, `mousePressEvent`, `enterEvent`
  (cu `QtCompat::QEnterEvent`), `leaveEvent`, `keyPressEvent`, `keyReleaseEvent`.
- Include-uri obligatorii: `Global/Macros.h`, `Global/QtCompat.h`,
  `Gui/PanelWidget.h`, `Gui/GuiFwd.h` (`ScriptEditor.h:29-41`).

### `PanelWidget` — ce moștenim

- `Gui/PanelWidget.h:43-44` — `class PanelWidget : public ScriptObject`, cu membrii
  `QWidget* _thisWidget` și `Gui* _gui`.
- `Gui/PanelWidget.h:84` — `virtual void pushUndoCommand(QUndoCommand*)` — **public**.
- `Gui/PanelWidget.h:93` — `virtual QUndoStack* getUndoStack() const { return 0; }`
  — **protected**, întoarce `0` implicit.
- `Gui/PanelWidget.cpp:108-119` — implementarea: ia `getUndoStack()`, dacă e non-null
  face `stack->setActive()` apoi `stack->push(command)`; altfel `assert(false)`.
  **Deci un panou care vrea undo trebuie să suprascrie `getUndoStack()`.**
- `ScriptObject` (`Engine/ScriptObject.h:46,50,52`) dă `setLabel(std::string)`,
  `setScriptName(std::string)`, `getScriptName()`.

## 2. Instanțiere și înregistrare

- `Gui/GuiPrivate.h:247` — membrul: `ScriptEditor* _scriptEditor;` (pointer brut).
- `Gui/GuiPrivate.h:284` — declarația `void createScriptEditorGui();`.
- `Gui/GuiPrivate.cpp:421-429` — corpul, tiparul exact de copiat:

```cpp
void
GuiPrivate::createScriptEditorGui()
{
    _scriptEditor = new ScriptEditor(_gui);
    _scriptEditor->setScriptName("scriptEditor");
    _scriptEditor->setLabel( tr("Script Editor").toStdString() );
    _scriptEditor->setVisible(false);
    _gui->registerTab(_scriptEditor, _scriptEditor);
}
```

Notă: `registerTab` primește **același pointer de două ori** — o dată ca
`PanelWidget*`, o dată ca `ScriptObject*` — pentru că `PanelWidget` derivă din
`ScriptObject`.

- `Gui/Gui.h:242` — `void registerTab(PanelWidget* tab, ScriptObject* obj);`
  (`unregisterTab` la `:243`).
- `Gui/Gui05.cpp:122` — **singurul** loc de unde e apelată `createScriptEditorGui()`.
  Aici se adaugă și apelul nostru.
- `Gui/Gui.h:355` — accesorul public `ScriptEditor* getScriptEditor() const;`
- `Gui/Gui40.cpp:551` — implementarea accesorului.

## 3. Serializarea panoului în layout

- `Gui/Gui40.cpp:608` — `return _imp->_registeredTabs;` — harta de tab-uri
  înregistrate e sursa de adevăr.
- Mecanismul: `registerTab` bagă panoul în `_registeredTabs` **cheiat pe
  `getScriptName()`**. La restaurarea layout-ului, Natron caută panoul după acel
  script name. **Consecință: `setScriptName("aiChatPanel")` trebuie să fie stabil
  între versiuni** — dacă se schimbă, layout-urile salvate pierd panoul.
- `setVisible(false)` la creare e corect: panoul există și e înregistrat, dar nu se
  arată până nu îl cere utilizatorul din meniu sau până nu îl restaurează layout-ul.

## 4. Undo — cum grupez N mutații într-un singur Ctrl+Z

Răspunsul concret, în patru părți:

**(a) Unde e stiva.** `Gui/NodeGraphPrivate.h:181` — `QUndoStack* _undoStack;`
`Gui/NodeGraph.cpp:450-453` — `NodeGraph::getUndoStack()` întoarce `_imp->_undoStack`.
`Gui/NodeGraph.h:278` — e `OVERRIDE FINAL`.
Stiva se înregistrează global prin `Gui::registerNewUndoStack(QUndoStack*)`
(`Gui/Gui.h:162`, implementare `Gui/Gui30.cpp:649-657`), care o adaugă în
`_undoStacksGroup` și îi creează acțiunile Undo/Redo cu `QKeySequence::Undo/Redo`.

**(b) Mecanismul de grupare: `QUndoStack::beginMacro()` / `endMacro()`.**
Verificat: `grep -rn "beginMacro\|endMacro" Gui/ Engine/` întoarce **zero rezultate**
— tiparul nu e folosit încă nicăieri în Natron. Nu e interzis, doar nefolosit: e API
Qt standard pe `QUndoStack`, iar stiva NodeGraph-ului e un `QUndoStack` obișnuit.
Semantica: între `beginMacro(text)` și `endMacro()`, fiecare `push()` **execută**
comanda normal (`redo()` se apelează), dar comenzile devin copii ai unei comenzi
compuse; la final stiva conține **o singură** intrare cu textul dat. Exact ce ne
trebuie.

**(c) Capcanele — de asta trebuie RAII.**

1. `beginMacro` fără `endMacro` lasă macro-ul deschis **permanent**: stiva rămâne
   blocată, acțiunile Undo/Redo rămân dezactivate, iar orice comandă ulterioară intră
   în macro-ul orfan. Un `return` timpuriu sau o excepție între ele strică undo-ul
   pentru tot restul sesiunii.
2. Macro-urile se pot imbrica; trebuie contorizate dacă un tool cheamă altul.
3. `PanelWidget::pushUndoCommand` face `setActive()` la fiecare push — inofensiv
   într-un macro, dar înseamnă că trebuie să împingem pe **aceeași** stivă tot timpul,
   altfel comenzile se împrăștie pe stive diferite și gruparea se pierde.

→ Implementarea corectă e un guard RAII (`AIUndoTransaction`) care apelează
`endMacro()` în destructor, plus un contor de imbricare.

**(d) Comenzile existente de reutilizat** (`Gui/NodeGraphUndoRedo.h`):

- `:83` `AddMultipleNodesCommand(NodeGraph*, const NodesGuiList&, QUndoCommand* parent = 0)`
  și overload cu un singur `const NodeGuiPtr&` (`:93`).
- `:113` `RemoveMultipleNodesCommand`
- `:144` `ConnectCommand(NodeGraph*, Edge*, const NodeGuiPtr& oldSrc, const NodeGuiPtr& newSrc, QUndoCommand* parent = 0)`
- alte: `MoveMultipleNodesCommand:53`, `InsertNodeCommand:174`,
  `RenameNodeUndoRedoCommand:340`, `DisableNodesCommand:278`, `EnableNodesCommand:295`.

**Observație importantă:** aceste comenzi lucrează pe obiecte **GUI**
(`NodeGuiPtr`, `Edge*`), nu pe `Node`/`EffectInstance` din Engine. Deci serverul MCP
trebuie să facă maparea:

- `Node` → `NodeGui`: `Engine/Node.h:594` `NodeGuiIPtr getNodeGui() const;`, apoi
  `std::dynamic_pointer_cast<NodeGui>(...)` (legal, suntem în `Gui/`).
- input `i` → `Edge*`: `Gui/NodeGui.h:218` `Edge* getInputArrow(int inputNb) const;`
  (`getOutputArrow()` la `:217`).

## 5. Creare de nod din C++ și setare de knob

Tiparul (ex. `Gui/NodeGraph30.cpp:92-94`):

```cpp
CreateNodeArgs args( PLUGINID_NATRON_VIEWER, getGroup() );
NodePtr node = getGui()->getApp()->createNode(args);
if (!node) { /* eșec — createNode întoarce NULL, nu aruncă */ }
```

- Proprietăți utile (`Engine/CreateNodeArgs.h`): `kCreateNodeArgsPropPluginID:42`,
  `kCreateNodeArgsPropPluginVersion:49`, `kCreateNodeArgsPropNodeInitialPosition:57`
  (2 × double), `kCreateNodeArgsPropNodeInitialName:65`,
  `kCreateNodeArgsPropOutOfProject:109`, `kCreateNodeArgsPropNoNodeGUI:117`,
  `kCreateNodeArgsPropSettingsOpened:125`, `kCreateNodeArgsPropAutoConnect:132`,
  **`kCreateNodeArgsPropAddUndoRedoCommand:140`**.
- API: `args.setProperty<T>(name, value, index = 0, failIfNotExisting = true)`
  (`Engine/CreateNodeArgs.h:340`) și `setPropertyN<T>(name, vector)` (`:355`).
- **`kCreateNodeArgsPropAddUndoRedoCommand`** e cheia pentru Prompt 3: dacă e `true`,
  Natron împinge singur comanda de undo pentru creare. Combinat cu `beginMacro`, nu
  mai trebuie să construim manual `AddMultipleNodesCommand`.
- Knob-uri: se iau cu `node->getKnobByName(std::string)` și se setează cu
  `knob->setValue(v, ViewSpec::all(), dimension)` (tiparul din
  `Tests/BaseTest.cpp:250-253`).

## 6. Proces copil + IPC — tiparul existent

- `Engine/ProcessHandler.h:83-96` — `class ProcessHandler : public QObject` cu
  membrii: `QProcess* _process`, `QLocalServer* _ipcServer`,
  `QLocalSocket* _bgProcessOutputSocket`.
- `Engine/ProcessHandler.h:75-82` — comentariul descrie protocolul: mesajele sunt
  **exact o linie**, terminată cu `\n`. Adică deja NDJSON-friendly.
- `Engine/ExistenceCheckThread.h/.cpp` — heartbeat pe `QLocalSocket`, ca procesul
  copil să detecteze moartea părintelui.
- **Limitare de reținut:** `ProcessHandler` se bazează pe semnale Qt în coadă
  (`QLocalServer::newConnection`), deci funcționează **doar** cu event loop — adică
  în `Natron` (GUI), nu în `NatronRenderer`. Pentru panoul nostru e în regulă:
  suntem în procesul GUI.

## Concluzii pentru implementare

1. Panoul se modelează 1:1 după `ScriptEditor`; singura parte nouă e suprascrierea
   lui `getUndoStack()` ca să întoarcă stiva NodeGraph-ului.
2. Gruparea undo = `beginMacro`/`endMacro` pe stiva NodeGraph-ului, cu guard RAII.
   Tipar nou în codebase, dar API Qt standard.
3. Serverul MCP trebuie să traducă script name → `Node` → `NodeGui` → `Edge`, pentru
   că undo-ul e la nivel de GUI.
4. Toate mutațiile trebuie să ajungă pe thread-ul GUI: `Node.cpp` are ~28 de
   `assert(QThread::currentThread() == qApp->thread())` care în release **dispar**,
   deci o încălcare nu crapă — corupe tăcut.
