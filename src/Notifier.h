#pragma once

#include "Notification.h"

#include <vector>
#include <functional>
#include <map>
#include <memory>
#include <algorithm>

namespace Dexode
{

template<typename T>
struct notifier_traits
{
	typedef T type;
};

class Notifier
{
public:
	static Notifier& getGlobal()
	{
		static Notifier globalNotifier;
		return globalNotifier;
	}

	Notifier() = default;

	virtual ~Notifier()
	{
		_callbacks.clear();
	}

	Notifier(const Notifier&) = delete;
	Notifier(Notifier&&) = delete;

	Notifier& operator=(Notifier&&) = delete;
	Notifier& operator=(const Notifier&) = delete;

	/**
	 * Register listener for notification. Returns token used to unregister
	 *
	 * @param notification - pass notification like "getNotificationXYZ()"
	 * @param callback - your callback to handle notification
	 * @return token used to unregister
	 */
	template<typename ... Args>
	int listen(const Notification<Args...>& notification
			, typename notifier_traits<const std::function<void(Args...)>&>::type callback)
	{
		const int token = ++_tokener;
		listen(token, notification, callback);
		return token;
	}

	/**
	 * @param token - unique token for identification receiver. Simply pass token from @see Notifier::listen
	 * @param notification - pass notification like "getNotificationXYZ()"
	 * @param callback - your callback to handle notification
	 */
	template<typename ... Args>
	void listen(const int token
			, const Notification<Args...>& notification
			, typename notifier_traits<const std::function<void(Args...)>&>::type callback)
	{
		using CallbackType = std::function<void(Args...)>;
		using Vector = VectorImpl<CallbackType>;

		//Check for valid object
		assert(callback && "Please set it");

		if (_callbacks[notification.tag] == nullptr)
		{
			_callbacks[notification.tag] = std::unique_ptr<VectorInterface>(new Vector{});
		}

		auto pVector = static_cast<Vector*>(_callbacks[notification.tag].get());
		assert(pVector);
		pVector->container.emplace_back(std::make_pair(callback, token));
	}

	/**
	 * @param token - token from Notifier::listen
	 */
	void unlistenAll(const int token)
	{
		for (auto&& element : _callbacks)
		{
			element.second->remove(token);
		}
	}

	/**
	 * @param token - token from Notifier::listen
	 * @param notification - notification you wan't to unlisten. @see Notiier::listen
	 */
	template<typename NotificationType, typename ... Args>
	void unlisten(const int token, const NotificationType& notification)
	{
		using CallbackType = typename notification_traits<NotificationType>::callback_type;
		using Vector = VectorImpl<CallbackType>;
		assert(notification.tag > NotificationConst::UNUSED_TAG);

		if (_callbacks[notification.tag] == nullptr)
		{
			return;
		}
		assert(dynamic_cast<Vector*>(_callbacks[notification.tag].get()) != nullptr);
		_callbacks[notification.tag]->remove(token);
	}

	template<typename NotificationType, typename ... Args>
	void notify(const NotificationType& notification, Args&& ... params)
	{
		using CallbackType = typename notification_traits<NotificationType>::callback_type;
		using Vector = VectorImpl<CallbackType>;
		assert(notification.tag > NotificationConst::UNUSED_TAG);

		if (_callbacks.find(notification.tag) == _callbacks.end())
		{
			return;
		}

		assert(dynamic_cast<Vector*>(_callbacks[notification.tag].get()) != nullptr);

		//Copy? TODO think about it Use 2 vectors?
		auto pVector = static_cast<Vector*>(_callbacks[notification.tag].get());
		for (auto&& element : pVector->container)
		{
			element.first(std::forward<Args>(params)...);
		}
	}

private:
	struct VectorInterface
	{
		virtual ~VectorInterface() = default;

		virtual void remove(const int token) = 0;
		virtual void removeAll() = 0;
	};

	template<typename Type>
	struct VectorImpl : public VectorInterface
	{
		std::vector<std::pair<Type, int>> container;

		virtual ~VectorImpl()
		{
			removeAll();
		}

		virtual void removeAll() override
		{
			container.clear();
		}

		virtual void remove(const int token) override
		{
			auto removeFrom = std::remove_if(container.begin(), container.end()
											 , [token](const std::pair<Type, int>& element)
					{
						return element.second == token;
					});
			if (removeFrom == container.end())
			{
				return;
			}
			container.erase(removeFrom, container.end());
		}
	};

	int _tokener = 0;
	std::map<int, std::unique_ptr<VectorInterface>> _callbacks;
};

} /* namespace Dexode */
