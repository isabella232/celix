/*
 * remote_proxy_factory_impl.c
 *
 *  Created on: 22 Dec 2014
 *      Author: abroekhuis
 */

#include <stdlib.h>

#include "remote_proxy.h"

typedef struct proxy_instance {
	service_registration_pt registration_ptr;
	void *service;
	properties_pt properties;
} *proxy_instance_pt;

static celix_status_t remoteProxyFactory_registerProxyService(remote_proxy_factory_pt remote_proxy_factory_ptr, endpoint_description_pt endpointDescription, remote_service_admin_pt rsa, sendToHandle sendToCallback);
static celix_status_t remoteProxyFactory_unregisterProxyService(remote_proxy_factory_pt remote_proxy_factory_ptr, endpoint_description_pt endpointDescription);

celix_status_t remoteProxyFactory_create(bundle_context_pt context, char *service, void *handle,
		createProxyService create, destroyProxyService destroy,
		remote_proxy_factory_pt *remote_proxy_factory_ptr) {
	celix_status_t status = CELIX_SUCCESS;

	*remote_proxy_factory_ptr = calloc(1, sizeof(**remote_proxy_factory_ptr));
	if (!*remote_proxy_factory_ptr) {
		status = CELIX_ENOMEM;
	}

	if (status == CELIX_SUCCESS) {
		(*remote_proxy_factory_ptr)->context_ptr = context;
		(*remote_proxy_factory_ptr)->service = strdup(service);

		(*remote_proxy_factory_ptr)->remote_proxy_factory_service_ptr = NULL;
		(*remote_proxy_factory_ptr)->properties = NULL;
		(*remote_proxy_factory_ptr)->registration = NULL;

		(*remote_proxy_factory_ptr)->proxy_instances = hashMap_create(NULL, NULL, NULL, NULL);

		(*remote_proxy_factory_ptr)->handle = handle;

		(*remote_proxy_factory_ptr)->create_proxy_service_ptr = create;
		(*remote_proxy_factory_ptr)->destroy_proxy_service_ptr = destroy;
	}

	return status;
}

celix_status_t remoteProxyFactory_destroy(remote_proxy_factory_pt *remote_proxy_factory_ptr) {
	celix_status_t status = CELIX_SUCCESS;

	if (!*remote_proxy_factory_ptr) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		if ((*remote_proxy_factory_ptr)->proxy_instances) {
			hashMap_destroy((*remote_proxy_factory_ptr)->proxy_instances, false, false);
			(*remote_proxy_factory_ptr)->proxy_instances = NULL;
		}
		if ((*remote_proxy_factory_ptr)->service) {
			free((*remote_proxy_factory_ptr)->service);
			(*remote_proxy_factory_ptr)->service = NULL;
		}
		free(*remote_proxy_factory_ptr);
		*remote_proxy_factory_ptr = NULL;
	}

	return status;
}

celix_status_t remoteProxyFactory_register(remote_proxy_factory_pt remote_proxy_factory_ptr) {
	celix_status_t status = CELIX_SUCCESS;

	remote_proxy_factory_ptr->remote_proxy_factory_service_ptr = calloc(1, sizeof(*remote_proxy_factory_ptr->remote_proxy_factory_service_ptr));
	if (!remote_proxy_factory_ptr->remote_proxy_factory_service_ptr) {
		status = CELIX_ENOMEM;
	}

	if (status == CELIX_SUCCESS) {
		remote_proxy_factory_ptr->remote_proxy_factory_service_ptr->factory = remote_proxy_factory_ptr;
		remote_proxy_factory_ptr->remote_proxy_factory_service_ptr->registerProxyService = remoteProxyFactory_registerProxyService;
		remote_proxy_factory_ptr->remote_proxy_factory_service_ptr->unregisterProxyService = remoteProxyFactory_unregisterProxyService;

		remote_proxy_factory_ptr->properties = properties_create();
		if (!remote_proxy_factory_ptr->properties) {
			status = CELIX_BUNDLE_EXCEPTION;
		} else {
			properties_set(remote_proxy_factory_ptr->properties, "proxy.interface", remote_proxy_factory_ptr->service);
		}
	}

	if (status == CELIX_SUCCESS) {
		status = bundleContext_registerService(remote_proxy_factory_ptr->context_ptr, OSGI_RSA_REMOTE_PROXY_FACTORY,
				remote_proxy_factory_ptr->remote_proxy_factory_service_ptr, remote_proxy_factory_ptr->properties, &remote_proxy_factory_ptr->registration);
	}

	return status;
}

celix_status_t remoteProxyFactory_unregister(remote_proxy_factory_pt remote_proxy_factory_ptr) {
	celix_status_t status = CELIX_SUCCESS;

	if (!remote_proxy_factory_ptr) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	// #TODO Remove proxy registrations
	if (status == CELIX_SUCCESS) {
		if (remote_proxy_factory_ptr->registration) {
			status = serviceRegistration_unregister(remote_proxy_factory_ptr->registration);
			remote_proxy_factory_ptr->properties = NULL;
		}
		if (remote_proxy_factory_ptr->properties) {
			properties_destroy(remote_proxy_factory_ptr->properties);
		}
		if (remote_proxy_factory_ptr->remote_proxy_factory_service_ptr) {
			free(remote_proxy_factory_ptr->remote_proxy_factory_service_ptr);
		}
	}

	return status;
}

static celix_status_t remoteProxyFactory_registerProxyService(remote_proxy_factory_pt remote_proxy_factory_ptr, endpoint_description_pt endpointDescription, remote_service_admin_pt rsa, sendToHandle sendToCallback) {
	celix_status_t status = CELIX_SUCCESS;
	proxy_instance_pt proxy_instance_ptr = NULL;

	if (!remote_proxy_factory_ptr || !remote_proxy_factory_ptr->create_proxy_service_ptr) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		proxy_instance_ptr = calloc(1, sizeof(*proxy_instance_ptr));
		if (!proxy_instance_ptr) {
			status = CELIX_ENOMEM;
		}
	}

	if (status == CELIX_SUCCESS) {
		proxy_instance_ptr->properties = properties_create();
		if (!proxy_instance_ptr->properties) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if (status == CELIX_SUCCESS) {
		status = remote_proxy_factory_ptr->create_proxy_service_ptr(remote_proxy_factory_ptr->handle, endpointDescription, rsa, sendToCallback, proxy_instance_ptr->properties, &proxy_instance_ptr->service);
	}

	if (status == CELIX_SUCCESS) {
		properties_set(proxy_instance_ptr->properties, "proxy.interface", remote_proxy_factory_ptr->service);
		properties_set(proxy_instance_ptr->properties, "endpoint.framework.uuid", (char *) endpointDescription->frameworkUUID);

		// #TODO Copy properties from the endpoint
	}

	if (status == CELIX_SUCCESS) {
		status = bundleContext_registerService(remote_proxy_factory_ptr->context_ptr, remote_proxy_factory_ptr->service, proxy_instance_ptr->service, proxy_instance_ptr->properties, &proxy_instance_ptr->registration_ptr);
	}

	if (status == CELIX_SUCCESS) {
		hashMap_put(remote_proxy_factory_ptr->proxy_instances, endpointDescription, proxy_instance_ptr);
	}

	return status;
}

static celix_status_t remoteProxyFactory_unregisterProxyService(remote_proxy_factory_pt remote_proxy_factory_ptr, endpoint_description_pt endpointDescription) {
	celix_status_t status = CELIX_SUCCESS;
	proxy_instance_pt proxy_instance_ptr = NULL;

	if (!remote_proxy_factory_ptr || !endpointDescription || !remote_proxy_factory_ptr->proxy_instances || !remote_proxy_factory_ptr->handle) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		proxy_instance_ptr = hashMap_remove(remote_proxy_factory_ptr->proxy_instances, endpointDescription);
		if (proxy_instance_ptr == NULL) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if (status == CELIX_SUCCESS) {
		if (proxy_instance_ptr->registration_ptr) {
			status = serviceRegistration_unregister(proxy_instance_ptr->registration_ptr);
			proxy_instance_ptr->properties = NULL;
		}
		if (proxy_instance_ptr->service) {
			status = remote_proxy_factory_ptr->destroy_proxy_service_ptr(remote_proxy_factory_ptr->handle, proxy_instance_ptr->service);
		}
		if (proxy_instance_ptr->properties) {
			properties_destroy(proxy_instance_ptr->properties);
		}
		if (proxy_instance_ptr) {
			free(proxy_instance_ptr);
		}
	}

	return status;
}


