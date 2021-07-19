#include "zaf/actor_behavior.hpp"
#include "zaf/actor_system.hpp"
#include "zaf/scoped_actor.hpp"

namespace zaf {
// ActorSystem::ActorSystem(ActorSystem&& other) {
//   this->zmq_context.close();
//   this->zmq_context = std::move(other.zmq_context);
//   this->num_alive_actors = other.num_alive_actors.load();
//   other.num_alive_actors = 0;
//   this->next_available_actor_id = other.next_available_actor_id.load();
//   other.next_available_actor_id = 0;
// }

// ActorSystem& ActorSystem::operator=(ActorSystem&& other) {
//   this->zmq_context.close();
//   this->zmq_context = std::move(other.zmq_context);
//   this->num_alive_actors = other.num_alive_actors.load();
//   other.num_alive_actors = 0;
//   this->next_available_actor_id = other.next_available_actor_id.load();
//   other.next_available_actor_id = 0;
// }

ScopedActor ActorSystem::create_scoped_actor() {
  auto new_actor = new ActorBehavior();
  new_actor->initialize_actor(*this);
  return {new_actor};
}

zmq::context_t& ActorSystem::get_zmq_context() {
  return zmq_context;
}

ActorIdType ActorSystem::get_next_available_actor_id() {
  return next_available_actor_id.fetch_add(1, std::memory_order::memory_order_relaxed);
}

ActorSystem::~ActorSystem() {
  this->await_all_actors_done();
  zmq_context.close();
}
} // namespace zaf
