#include "node/node.h"
#include "nodebuilder.h"
#include "nodeevents.h"

YAML_NAMESPACE_ENTER
Node Clone(const Node& node) {
  NodeEvents events(node);
  NodeBuilder builder;
  events.Emit(builder);
  return builder.Root();
}
YAML_NAMESPACE_EXIT
