# ngx_http_rate_limit_request_module
A rate limit module for Nginx

## Nginx Version
1.24.0

## Introduction

The ```ngx_http_rate_limit_request_module``` module implements rate limit functionality based on the result of a subrequest. If the subrequest returns a 2xx response code, the access is allowed. If it returns 429 or 403, the access is denied with the corresponding error code. Any other response code returned by the subrequest is considered an error.

It's a important component of this [service](https://github.com/ralgond/rate-limit-server)

## Example Configuration
```bash
location / {
    rate_limit_request /rate_limit;
    ...
}

location = /rate_limit {
    internal;
    proxy_pass ... # proxy_pass http://backend_ratelimit$request_uri
    proxy_http_version 1.1;
    proxy_set_header X-Real-IP $remote_addr;
    proxy_set_header X-Real-Method $request_method;
    proxy_set_header X-Original-URI $request_uri;
    proxy_set_header Content-Type "";
    proxy_set_header Content-Length 0;
    proxy_set_header Connection "keep-alive";
    proxy_pass_request_body off;
}
```
## Directives
- Syntax:  **rate_limit_request** *uri* | *off*;
- Default:  **rate_limit_request** *off*;
- Context:  http, server, location

Enables rate limit functionality based on the result of a subrequest and sets the URI to which the subrequest will be sent.

