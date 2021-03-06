// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_GCM_ENGINE_CONNECTION_FACTORY_IMPL_H_
#define GOOGLE_APIS_GCM_ENGINE_CONNECTION_FACTORY_IMPL_H_

#include "google_apis/gcm/engine/connection_factory.h"

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "google_apis/gcm/protocol/mcs.pb.h"
#include "net/base/backoff_entry.h"
#include "net/base/network_change_notifier.h"
#include "net/proxy/proxy_info.h"
#include "net/proxy/proxy_service.h"
#include "net/socket/client_socket_handle.h"
#include "url/gurl.h"

namespace net {
class HttpNetworkSession;
class NetLog;
}

namespace gcm {

class ConnectionHandlerImpl;

class GCM_EXPORT ConnectionFactoryImpl :
    public ConnectionFactory,
    public net::NetworkChangeNotifier::ConnectionTypeObserver,
    public net::NetworkChangeNotifier::IPAddressObserver {
 public:
  ConnectionFactoryImpl(
      const GURL& mcs_endpoint,
      const net::BackoffEntry::Policy& backoff_policy,
      scoped_refptr<net::HttpNetworkSession> network_session,
      net::NetLog* net_log);
  virtual ~ConnectionFactoryImpl();

  // ConnectionFactory implementation.
  virtual void Initialize(
      const BuildLoginRequestCallback& request_builder,
      const ConnectionHandler::ProtoReceivedCallback& read_callback,
      const ConnectionHandler::ProtoSentCallback& write_callback) OVERRIDE;
  virtual ConnectionHandler* GetConnectionHandler() const OVERRIDE;
  virtual void Connect() OVERRIDE;
  virtual bool IsEndpointReachable() const OVERRIDE;
  virtual base::TimeTicks NextRetryAttempt() const OVERRIDE;
  virtual void SignalConnectionReset(ConnectionResetReason reason) OVERRIDE;

  // NetworkChangeNotifier observer implementations.
  virtual void OnConnectionTypeChanged(
      net::NetworkChangeNotifier::ConnectionType type) OVERRIDE;
  virtual void OnIPAddressChanged() OVERRIDE;

 protected:
  // Implementation of Connect(..). If not in backoff, uses |login_request_|
  // in attempting a connection/handshake. On connection/handshake failure, goes
  // into backoff.
  // Virtual for testing.
  virtual void ConnectImpl();

  // Helper method for initalizing the connection hander.
  // Virtual for testing.
  virtual void InitHandler();

  // Helper method for creating a backoff entry.
  // Virtual for testing.
  virtual scoped_ptr<net::BackoffEntry> CreateBackoffEntry(
      const net::BackoffEntry::Policy* const policy);

  // Returns the current time in Ticks.
  // Virtual for testing.
  virtual base::TimeTicks NowTicks();

  // Callback for Socket connection completion.
  void OnConnectDone(int result);

  // ConnectionHandler callback for connection issues.
  void ConnectionHandlerCallback(int result);

 private:
  // Proxy resolution and connection functions.
  void OnProxyResolveDone(int status);
  void OnProxyConnectDone(int status);
  int ReconsiderProxyAfterError(int error);
  void ReportSuccessfulProxyConnection();

  void CloseSocket();

  // The MCS endpoint to make connections to.
  const GURL mcs_endpoint_;

  // The backoff policy to use.
  const net::BackoffEntry::Policy backoff_policy_;

  // ---- net:: components for establishing connections. ----
  // Network session for creating new connections.
  const scoped_refptr<net::HttpNetworkSession> network_session_;
  // Net log to use in connection attempts.
  net::BoundNetLog bound_net_log_;
  // The current PAC request, if one exists. Owned by the proxy service.
  net::ProxyService::PacRequest* pac_request_;
  // The current proxy info.
  net::ProxyInfo proxy_info_;
  // The handle to the socket for the current connection, if one exists.
  net::ClientSocketHandle socket_handle_;
  // Current backoff entry.
  scoped_ptr<net::BackoffEntry> backoff_entry_;
  // Backoff entry from previous connection attempt. Updated on each login
  // completion.
  scoped_ptr<net::BackoffEntry> previous_backoff_;

  // Whether a connection attempt is currently in progress or we're in backoff
  // waiting until the next connection attempt. |!connecting_| denotes
  // steady state with an active connection.
  bool connecting_;

  // Whether login successfully completed after the connection was established.
  // If a connection reset happens while attempting to log in, the current
  // backoff entry is reused (after incrementing with a new failure).
  bool logging_in_;

  // The time of the last login completion. Used for calculating whether to
  // restore a previous backoff entry and for measuring uptime.
  base::TimeTicks last_login_time_;

  // The current connection handler, if one exists.
  scoped_ptr<ConnectionHandlerImpl> connection_handler_;

  // Builder for generating new login requests.
  BuildLoginRequestCallback request_builder_;

  base::WeakPtrFactory<ConnectionFactoryImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionFactoryImpl);
};

}  // namespace gcm

#endif  // GOOGLE_APIS_GCM_ENGINE_CONNECTION_FACTORY_IMPL_H_
