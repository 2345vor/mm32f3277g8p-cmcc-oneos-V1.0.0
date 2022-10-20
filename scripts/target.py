import os
import sys
import shutil
import importlib
import importlib.util

def searchfile(path, name):
    projects = []
    L = len(os.path.abspath('.')) + 1
    for item in os.listdir(path):
        if os.path.isdir(os.path.join(path, item)):
            projects += searchfile(os.path.join(path, item), name)
        else:
            if name in item:
                projects.append(path[L:])
                
    return projects

def buildone(proj):    
    cwd = os.getcwd()
    proj_dir = os.path.join(cwd, proj)
    os.chdir(proj_dir)
    
    os.system('rm .sconsign.dblite')
    
    os.system('olddefconfig')

    if os.path.exists('template.uvprojx'):
        os.system('scons --ide=mdk5')    
    
    os.chdir(cwd)
    
def refresh_target(path):
    projects = searchfile(os.path.abspath(path), 'osconfig.py')
    
    for proj in projects:
        buildone(proj)
        
def target():
    global g
    g = {}
    soc_type = None
    
    os.system('menuconfig')

    file = open("oneos_config.h", 'r+')
    for ss in file.readlines(): 
        if '#define TEMP_' in ss:
            soc_type = ss.split('_')[1].replace('\n','')
        if '#define ' in ss:
            g[ss.split()[1]] = 1
           
    file.close()

    if soc_type == None:
        print("unknow soc_type")
        return None

    print("soc_type: %s" % soc_type)
    target_path = './' + soc_type
    
    if os.path.exists(target_path):
        print("soc_type: %s allready exist" % soc_type)
        return soc_type
    
    loader = importlib.machinery.SourceFileLoader('SConscript', "../templates/configs/SConscript")
    spec = importlib.util.spec_from_loader(loader.name, loader)
    mod = importlib.util.module_from_spec(spec)
    loader.exec_module(mod)
    source_path = mod.soc_type_inq(g)
    if source_path == None or not os.path.exists(source_path):
        print("not support %s" % soc_type)
        return None
    
    shutil.copytree(source_path, target_path)
    shutil.rmtree(source_path)
    
    refresh_target(target_path)
    
    return soc_type
