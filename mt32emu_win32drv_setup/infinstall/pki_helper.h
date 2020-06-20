#pragma once

const char ROOT_CERT_STORE_NAME[] = "Root";
const char TRUSTED_PUBLISHER_CERT_STORE_NAME[] = "TrustedPublisher";

extern "C" {
	BOOL SelfSignFile(LPCSTR szFileName, LPCSTR szCertSubject);
	BOOL RemoveCertFromStore(LPCSTR szCertSubject, LPCSTR szStoreName);
}
