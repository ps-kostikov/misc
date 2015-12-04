import os


def copy_with_hardlinks(src, dst):
    '''
    copy src directory to dst recursively using ln instead of cp
    behaves like cp -rl
    '''
    os.makedirs(dst)
    for f in os.listdir(src):
        src_path = os.path.join(src, f)
        dst_path = os.path.join(dst, f)
        if os.path.isdir(src_path):
            copy_with_hardlinks(src_path, dst_path)
        else:
            os.link(src_path, dst_path)


copy_with_hardlinks('1', '2')
