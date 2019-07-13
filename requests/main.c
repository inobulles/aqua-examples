
// simple program that sets the user agent to something like "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit Chrome/72",
// makes a HTTP GET request to https://baconipsum.com/api/?type=meat-and-filler&paras=5&format=text,
// and prints out the result

#include "root.h"
#include "lib/extensions/requests.h"

bool main(void) {
	request_t request;
	
	request_user_agent(REQUEST_USER_AGENT_MODERN_BROWSER);
	if (request_get(&request, "https://baconipsum.com/api/?type=meat-and-filler&paras=5&format=text")) {
		print("WARNING Request failed (error %lld)\n", request.code);
		return true;
		
	}
	
	print("%s\n", request.data);
	print("\n(%lld bytes)\n", request.bytes);
	
	request_free(&request);
	return false;
	
}
