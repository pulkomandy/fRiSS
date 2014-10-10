class BUrlProtocolListener;

void LoadFeedNet(const char* feed, BUrlProtocolListener* listener);

int LoadFile(char* buf, int bufsize, const char* szProgram, const char* argv[] );
int LoadFile(char* buf, int bufsize, const char* szArgs);
int LoadFortune(char* buf, int bufsize);
