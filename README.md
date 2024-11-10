# ngx_http_rate_limit_request_module
A rate limit module for Nginx

## Nginx Version
1.24.0

## Introduction

The ```ngx_http_rate_limit_request_module``` module implements rate limit functionality based on the result of a subrequest. If the subrequest returns a 2xx response code, the access is allowed. If it returns 429 or 403, the access is denied with the corresponding error code. Any other response code returned by the subrequest is considered an error.

## Example Configuration
```bash
location / {
    rate_limit_request /rate_limit;
    ...
}

location = /rate_limit {
    internal;
    proxy_pass ... # proxy_pass http://backend_ratelimit$request_uri
    proxy_pass_request_body off;
    proxy_set_header Content-Length "";
    proxy_set_header X-Original-URI $request_uri;
}
```
## Directives
- Syntax:  **rate_limit_request** *uri* | *off*;
- Default:  **rate_limit_request** *off*;
- Context:  http, server, location

Enables rate limit functionality based on the result of a subrequest and sets the URI to which the subrequest will be sent.

