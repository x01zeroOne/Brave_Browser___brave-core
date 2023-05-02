/* Copyright (c) 2022 The Flower Authors. */

#include "brave/third_party/flower/src/cc/flwr/include/message_handler.h"

std::tuple<ClientMessage, int> _Reconnect(
    ServerMessage_Reconnect reconnect_msg) {
  // Determine the reason for sending Disconnect message
  Reason reason = Reason::ACK;
  int sleep_duration = 0;
  if (reconnect_msg.seconds() != 0) {
    reason = Reason::RECONNECT;
    sleep_duration = reconnect_msg.seconds();
  }

  // Build Disconnect message
  ClientMessage_Disconnect disconnect;
  disconnect.set_reason(reason);
  ClientMessage cm;
  *cm.mutable_disconnect() = disconnect;

  return std::make_tuple(cm, sleep_duration);
}

ClientMessage _GetParameters(flwr::Client* client) {
  ClientMessage cm;
  *(cm.mutable_parameters_res()) =
      parameters_res_to_proto(client->GetParameters());
  return cm;
}

ClientMessage _Fit(flwr::Client* client, ServerMessage_FitIns fit_msg) {
  // Deserialize fit instruction
  flwr::FitIns fit_ins = fit_ins_from_proto(fit_msg);
  // Perform fit
  flwr::FitRes fit_res = client->Fit(fit_ins);
  // Serialize fit result
  ClientMessage cm;
  *cm.mutable_fit_res() = fit_res_to_proto(fit_res);
  return cm;
}

ClientMessage _Evaluate(flwr::Client* client,
                        ServerMessage_EvaluateIns evaluate_msg) {
  // Deserialize evaluate instruction
  flwr::EvaluateIns evaluate_ins = evaluate_ins_from_proto(evaluate_msg);
  // Perform evaluation
  flwr::EvaluateRes evaluate_res = client->Evaluate(evaluate_ins);
  // Serialize evaluate result
  ClientMessage cm;
  *cm.mutable_evaluate_res() = evaluate_res_to_proto(evaluate_res);
  return cm;
}

std::tuple<ClientMessage, int, bool> HandleMessage(flwr::Client* client,
                                            ServerMessage server_msg) {
  if (server_msg.has_reconnect()) {
    std::tuple<ClientMessage, int> rec = _Reconnect(server_msg.reconnect());
    return std::make_tuple(std::get<0>(rec), std::get<1>(rec), false);
  }
  if (server_msg.has_get_parameters()) {
    return std::make_tuple(_GetParameters(client), 0, true);
  }
  if (server_msg.has_fit_ins()) {
    return std::make_tuple(_Fit(client, server_msg.fit_ins()), 0, true);
  }
  if (server_msg.has_evaluate_ins()) {
    return std::make_tuple(_Evaluate(client, server_msg.evaluate_ins()), 0,
                           true);
  }
  throw "Unkown server message";
}
