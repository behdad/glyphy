if ANDROID

all:
	NDK_PROJECT_PATH=$(srcdir) ndk-build NDK_DEBUG=1
	ant -buildfile $(srcdir)/build.xml debug

clean-local:
	NDK_PROJECT_PATH=$(srcdir) ndk-build clean
	ant -buildfile $(srcdir)/build.xml clean

endif

GITIGNOREFILES = bin libs obj

-include $(top_srcdir)/git.mk
