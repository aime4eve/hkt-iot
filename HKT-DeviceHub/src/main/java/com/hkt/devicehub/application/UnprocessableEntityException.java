package com.hkt.devicehub.application;

/** Maps to HTTP 422 — the request is understood but refused by domain rules. */
public class UnprocessableEntityException extends RuntimeException {
    public UnprocessableEntityException(String message) {
        super(message);
    }
}
