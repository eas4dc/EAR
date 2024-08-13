import socket
import os
from ctypes import *

GENERIC_NAME = 256
ENERGY_TAG_SIZE = 32
POLICY_NAME = 32

def wrap_function(lib, func_name, res_type, arg_types):
    """Simplify wrapping ctypes functions"""
    func = lib.__getattr__(func_name)
    func.res_type = res_type
    func.argtypes = arg_types
    return func


class Job(Structure):
    _fields_ = [('id', c_ulong),
                ('step_id', c_ulong),
                ('user_id', c_char * GENERIC_NAME),
                ('group_id', c_char * GENERIC_NAME),
                ('app_id', c_char * GENERIC_NAME),
                ('user_acc', c_char * GENERIC_NAME),
                ('energy_tag', c_char * ENERGY_TAG_SIZE),
                ('start_time', c_ulong),
                ('end_time', c_ulong),
                ('start_mpi_time', c_ulong),
                ('end_mpi_time', c_ulong),
                ('policy', c_char * POLICY_NAME),
                ('th', c_double),
                ('procs', c_ulong),
                ('type', c_ulong),
                ('def_f', c_ulong)]

    def __repr__(self):
        return f'({self.id},{self.step_id},{self.user_id},{self.group_id},{self.user_acc},{self.energy_tag})'


class NewJobReq(Structure):
    _fields_ = [
                ('job', Job),
                ('is_mpi', c_uint8),
                ('is_learning', c_uint8)]
    def __repr__(self):
        return f'({self.job}, {self.is_mpi}, {self.is_learning})'


"""Load library"""
test = cdll.LoadLibrary('./libtest.so')
eard_rapi = cdll.LoadLibrary('./libeard_rapi.so')

"""Wrap C functions"""
print_job_req = wrap_function(test, 'print_job_req', None, [POINTER(NewJobReq)])
remote_connect = wrap_function(eard_rapi, 'remote_connect', None, [c_char_p, c_uint])
remote_disconnect = wrap_function(eard_rapi, 'remote_disconnect', None, None)
eards_new_job_nodelist = wrap_function(eard_rapi, 'eards_new_job_nodelist', None, [POINTER(NewJobReq), c_uint, c_char_p, POINTER(c_char_p), c_int])

"""Create working objects"""
job = Job(id=44, step_id=1 , user_id=b'xovidal', group_id=b'xbsc', user_acc=b'', energy_tag=b'', policy=b'', th=0)
job_req = NewJobReq(job=job, is_mpi=1, is_learning=0)

"""Do tests"""
ret = print_job_req(byref(job_req))
print(f'Value returned by print_job_req: {ret}')

nodename = socket.gethostname()
print(nodename)
port = 5000

print('calling remote connect')
ret = remote_connect(nodename.encode(), port)
print(f'Value returned by remote connect: {ret}')

try:
    print('calling new job nodelist')
    nodelist = c_char_p(nodename.encode())
    print(type(nodelist))
    nodelist_ptr = pointer(nodelist)
    print(nodelist_ptr)
    eards_new_job_nodelist(byref(job_req), port, b'', nodelist_ptr, 1)
except Exception as e:
    print('exception occurred')
    print('calling remote disconnect')
    ret = remote_disconnect()
    raise e

print('calling remote disconnect')
ret = remote_disconnect()
print(f'Value returned by remote disconnect: {ret}')

"""
print(c_int(5000))
ret = eard_rapi.remote_connect(nodename.encode(), c_int(5000))
print(ret)

ret = eard_rapi.eards_new_job_nodelist(byref(fill_new_job_data()), c_uint(5000), ''.encode(), pointer(nodename), c_int(1));

ret = eard_rapi.remote_disconnect()
print(ret)
"""
