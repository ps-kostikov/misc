import os
import stat


def chown_all_read_recursively(path):
    for root, dirs, files in os.walk(path):
        for f in files:
            fp = os.path.join(root, f)
            os.chmod(fp, os.stat(fp).st_mode | stat.S_IROTH)
        for d in dirs:
            dp = os.path.join(root, d)
            os.chmod(dp, os.stat(dp).st_mode | stat.S_IROTH)


chown_all_read_recursively('path')
