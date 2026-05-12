// Copyright © 2019 CCP ehf.

#pragma once

#include <memory>
#include <vector>

#include "CivetServer.h"
#include "prometheus/registry.h"

namespace prometheus_module {
	namespace detail {
		class MetricsHandler : public CivetHandler {
		public:
			MetricsHandler(const std::vector<std::weak_ptr<prometheus::Collectable>>& collectables,
				prometheus::Registry& registry);

			bool handleGet(CivetServer* server, struct mg_connection* conn) override;

		private:
			std::vector<prometheus::MetricFamily> CollectMetrics() const;

			const std::vector<std::weak_ptr<prometheus::Collectable>>& collectables_;
			prometheus::Family<prometheus::Counter>& bytes_transferred_family_;
			prometheus::Counter& bytes_transferred_;
			prometheus::Family<prometheus::Counter>& num_scrapes_family_;
			prometheus::Counter& num_scrapes_;
			prometheus::Family<prometheus::Summary>& request_latencies_family_;
			prometheus::Summary& request_latencies_;
		};
	}  // namespace detail
}  // namespace prometheus

