#include <cassert>
#include <sstream>

#include "emitfromevents.h"
#include "emitter.h"
#include "emittermanip.h"
#include "null.h"

YAML_NAMESPACE_ENTER
struct Mark;
}  // YAML_NAMESPACE_ENTER

namespace {
std::string ToString(YAML_NAMESPACE::anchor_t anchor) {
  std::stringstream stream;
  stream << anchor;
  return stream.str();
}
}

YAML_NAMESPACE_ENTER
EmitFromEvents::EmitFromEvents(Emitter& emitter) : m_emitter(emitter) {}

void EmitFromEvents::OnDocumentStart(const Mark&) {}

void EmitFromEvents::OnDocumentEnd() {}

void EmitFromEvents::OnNull(const Mark&, anchor_t anchor) {
  BeginNode();
  EmitProps("", anchor);
  m_emitter << Null;
}

void EmitFromEvents::OnAlias(const Mark&, anchor_t anchor) {
  BeginNode();
  m_emitter << Alias(ToString(anchor));
}

void EmitFromEvents::OnScalar(const Mark&, const std::string& tag,
                              anchor_t anchor, const std::string& value) {
  BeginNode();
  EmitProps(tag, anchor);
  m_emitter << value;
}

void EmitFromEvents::OnSequenceStart(const Mark&, const std::string& tag,
                                     anchor_t anchor,
                                     EmitterStyle::value style) {
  BeginNode();
  EmitProps(tag, anchor);
  switch (style) {
    case EmitterStyle::Block:
      m_emitter << Block;
      break;
    case EmitterStyle::Flow:
      m_emitter << Flow;
      break;
    default:
      break;
  }
  m_emitter << BeginSeq;
  m_stateStack.push(State::WaitingForSequenceEntry);
}

void EmitFromEvents::OnSequenceEnd() {
  m_emitter << EndSeq;
  assert(m_stateStack.top() == State::WaitingForSequenceEntry);
  m_stateStack.pop();
}

void EmitFromEvents::OnMapStart(const Mark&, const std::string& tag,
                                anchor_t anchor, EmitterStyle::value style) {
  BeginNode();
  EmitProps(tag, anchor);
  switch (style) {
    case EmitterStyle::Block:
      m_emitter << Block;
      break;
    case EmitterStyle::Flow:
      m_emitter << Flow;
      break;
    default:
      break;
  }
  m_emitter << BeginMap;
  m_stateStack.push(State::WaitingForKey);
}

void EmitFromEvents::OnMapEnd() {
  m_emitter << EndMap;
  assert(m_stateStack.top() == State::WaitingForKey);
  m_stateStack.pop();
}

void EmitFromEvents::BeginNode() {
  if (m_stateStack.empty())
    return;

  switch (m_stateStack.top()) {
    case State::WaitingForKey:
      m_emitter << Key;
      m_stateStack.top() = State::WaitingForValue;
      break;
    case State::WaitingForValue:
      m_emitter << Value;
      m_stateStack.top() = State::WaitingForKey;
      break;
    default:
      break;
  }
}

void EmitFromEvents::EmitProps(const std::string& tag, anchor_t anchor) {
  if (!tag.empty() && tag != "?")
    m_emitter << VerbatimTag(tag);
  if (anchor)
    m_emitter << Anchor(ToString(anchor));
}
YAML_NAMESPACE_EXIT
