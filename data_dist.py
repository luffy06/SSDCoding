import os
import numpy as np
import csv
import struct
import random
import matplotlib.pyplot as plt
# import seaborn as sns

from statsmodels.compat import pandas

pagesize = 18432 # byte
pagenum = 1152
WLnum = 384

# import seaborn as sns
# from scipy import stats
# def read_from_csv(filename):
#     with open(filename+'.csv') as f:
#         f_csv = csv.reader(f)
#         headers = next(f_csv)
#         one_temp = []
#         for row in f_csv:
#             t = [1, 2]
#             t[0] = float(row[0])
#             t[1] = float(row[1])
#             one_temp.append(t)
#     one_temp = np.array(one_temp)
#     return one_temp

# names = ['cmix', 'enwik9', 'image', 'sound']
# names = ['cmix_new', 'enwik9_new', 'image_new', 'sound_new']

# i = 0
# for filename in names:
#     one_workload = read_from_csv(filename)
#     plt.subplot(420+i+1)
#     sns.distplot(one_workload[:, 0]+one_workload[:, 1], kde=False, fit=stats.gamma, rug=True)
#     plt.ylabel(filename)
#     # plt.hist(one_temp[:, 1])
#     # plt.subplot(420+i+2)
#     # sns.distplot(one_workload[:, 1], kde=False, fit=stats.gamma, rug=True)
#     # plt.subplot(223)
#     # sns.displot
#     i = i + 2
# plt.show()
# exit(0)

# from state value to separate byte in each page
# 8 state values to form 1 byte for each page (total 3 bytes)
def form_byte(v):
    r = np.zeros(3)
    p = 7
    for k in range(8):
        t = v[k]
        for i in range(3):
            b = t % 2
            t = t // 2
            r[2-i] = r[2-i] + b * pow(2, p)
        p = p - 1

    return r

# from 3 bytes to form 8 states
def form_state(L, C, M):
    s = np.zeros(8)
    for i in range(8):
        l = L & pow(2, 7 - i)
        l = l >> (7 - i)

        c = C & pow(2, 7 - i)
        c = c >> (7 - i)

        m = M & pow(2, 7 - i)
        m = m >> (7 - i)

        s[i] = l * 4 + c * 2 + m

    return s

def read_dist_file(fname):
    with open(fname) as f:
        f_csv = csv.reader(f)
        ret1 = []
        ret2 = []
        for row in f_csv:
            ret1.append(int(float(row[0])))
            ret2.append(int(float(row[1])))
    return ret1, ret2


def count_three(x, y, k, key): # count the percentage of key
    ret = 0
    for i in range(y.shape[0]):
        c = 0
        tmp = x[i]
        for j in range(k+1):
            cha = tmp // pow(10, k-j)
            tmp = tmp % pow(10, k-j)
            c = c + (cha == key)
        # print(c)
        ret = ret + c * y[i]
    return ret


def mapping(x, y, rep):
    mapx = list(range(0, y.shape[0], 1))
    for j in range(y.shape[0]):
        mapx[j] = x[j]
    for j in range(rep.__len__()):
        if j == y.shape[0]:
            break
        mapx[j] = rep[j]
        if rep[j] in x:
            idx = x.index(rep[j])
            mapx[idx] = x[j]
    return mapx


def state_dist(x, y, k):
    ret = np.zeros(y.shape[0] * (k + 1))
    idx = 0
    for i in range(y.shape[0]):
        tmp = x[i]
        for j in range(k + 1):
            cha = tmp // pow(10, k - j)
            tmp = tmp % pow(10, k - j)
            ret[idx] = cha
            idx = idx + 1
    return ret


type = ['cmix', 'enwik9', 'image', 'sound']
num = ['_one', '_two', '_three', '_four']
figs, axs = plt.subplots(2, 2, figsize=(12, 6),
                         gridspec_kw={
                             'left': 0.1,
                             'right': 0.95,
                             'top': 0.85,
                             'bottom': 0.25,
                             'wspace': 0.35,
                             'hspace': 0.4}
                         )
k = 0
rep = [3]


# for i in range(4):
#     print(type[i])
#     fname = 'encoding/backup_' + type[i] + num[k] + '_chara.csv'
#     x, y = read_dist_file(fname)
#     y.sort(reverse=True)
#     y = np.array(y)
#     y = y/sum(y)
#
#     # mapx = mapping(x, y, rep)
#     # a = count_three(x, y, k)
#     # b = count_three(mapx, y, k)
#     # print(a/(k+1), b/(k+1))
#
#     x = list(range(0, y.shape[0], 1))
#     ecdf = np.zeros(y.shape[0])
#     cur = 0
#     for j in range(y.shape[0]):
#         ecdf[j] = cur + y[j]
#         cur = ecdf[j]
#
#     axs[i // 2, i % 2].plot(x, ecdf, marker='.')
#     axs[i // 2, i % 2].set_title(type[i])
#
# plt.show()
#
# # print(x,y)
# exit(0)

def process_one_page():
    ret = 0
    return ret


fromname = 'D:/Research/compression/yuanhao-chang/164.gzip.filter'
fromname = 'workloads/backup_enwik9'
fromname = 'workloads/backup_sound'
# fromname = 'encoding/backup_cmix'
# fromname = 'encoding/backup_image'
print(fromname)

before = np.zeros(4)
after = np.zeros(4)

before4 = np.zeros(4)
after4 = np.zeros(4)

pagesize = 4096
pagesize = 16*1024
# pagesize = 40960
fromfile = open(fromname, 'rb')
Num = 100*1000*1000//(pagesize*3) - 1
# Num = 100
perc_3 = np.zeros((Num, 2))
for i in range(Num):
#     print(i)
    LSB = fromfile.read(pagesize)
    CSB = fromfile.read(pagesize)
    MSB = fromfile.read(pagesize)
    state_all = []
    for j in range(pagesize):
        state_all.append(form_state(LSB[j], CSB[j], MSB[j]))
    length = state_all.__len__()
    # print(length)
    state_all = np.array(state_all)
    state_all = np.reshape(state_all, -1)
    state_all = state_all.tolist()
    # print(state_all)
    x = []
    y = []
    for v in set(state_all):
        c = state_all.count(v)
        x.append(v)
        y.append(c)
    # print(x)

    xy = np.vstack((np.array(x), np.array(y)))
    xy = -xy.T
    xy = xy[xy[:, 1].argsort(-1)]
    xy = -xy
    x = list(xy[:, 0])
    y = xy[:, 1]
    y = y/sum(y)

    k = 0
    rep = [3]
    rep = [3, 4]
    mapx = mapping(x, y, rep)
    a = count_three(x, y, k, 3)
    b = count_three(mapx, y, k, 3)
    # print(a/(k+1), b/(k+1))
    before[0] = before[0] + a/(k+1)
    after[0] = after[0] + b/(k+1)

    a = count_three(x, y, k, 4)
    b = count_three(mapx, y, k, 4)
    # print(a/(k+1), b/(k+1))
    before4[0] = before4[0] + a/(k+1)
    after4[0] = after4[0] + b/(k+1)




    state_all2 = []
    for j in range(length // 2):
        state_all2.append(state_all[j * 2] * 10 + state_all[j * 2 + 1])
    x = []
    y = []
    for v in set(state_all2):
        c = state_all2.count(v)
        x.append(v)
        y.append(c)
    # print(x)
    xy = np.vstack((np.array(x), np.array(y)))
    xy = -xy.T
    xy = xy[xy[:, 1].argsort(-1)]
    xy = -xy
    x = list(xy[:, 0])
    y = xy[:, 1]
    y = y / sum(y)

    k = 1
    rep = [33, 30, 31, 32, 34, 35, 36, 37, 3, 13, 23, 43, 53, 63, 73]
    rep = [34, 43, 33, 44, 13, 14, 23, 24, 3, 4, 53, 54, 63, 64, 73, 74, 31, 41]
    mapx = mapping(x, y, rep)
    a = count_three(x, y, k, 3)
    b = count_three(mapx, y, k, 3)
    # print(a / (k + 1), b / (k + 1))
    before[1] = before[1] + a/(k+1)
    after[1] = after[1] + b/(k+1)

    a = count_three(x, y, k, 4)
    b = count_three(mapx, y, k, 4)
    # print(a/(k+1), b/(k+1))
    before4[1] = before4[1] + a/(k+1)
    after4[1] = after4[1] + b/(k+1)

    state_all3 = []
    # for j in range(length // 3):
    #     state_all3.append(state_all[j * 3] * 100 + state_all[j * 3 + 1] * 10 + state_all[j * 3 + 2])
    #
    # # for j in range(length // 8):
    # #     state_all3.append(state_all[j * 8] * 10000000 + state_all[j * 8 + 1] * 1000000 + state_all[j * 8 + 2] * 100000
    # #                       + state_all[j * 8 + 3] * 10000 + state_all[j * 8 + 4] * 1000 + state_all[j * 8 + 5] * 100
    # #                       + state_all[j * 8 + 6] * 10 + state_all[j * 8 + 7])
    # x = []
    # y = []
    # for v in set(state_all3):
    #     c = state_all3.count(v)
    #     x.append(v)
    #     y.append(c)
    # xy = np.vstack((np.array(x), np.array(y)))
    # xy = -xy.T
    # xy = xy[xy[:, 1].argsort(-1)]
    # xy = -xy
    # x = list(xy[:, 0])
    # y = xy[:, 1]
    #
    # y = y / sum(y)
    #
    # k = 2
    # # k = 7
    # rep = [333, 330, 331, 332, 334, 335, 336, 337, 33, 133, 233, 433, 533, 633, 733]
    # # rep = [33333333, 33333330, 33333331, 33333332, 33333334, 33333335, 33333336, 33333337, 3333333, 13333333,
    # #        23333333, 43333333, 53333333, 63333333, 73333333]
    # mapx = mapping(x, y, rep)
    # a = count_three(x, y, k)
    # b = count_three(mapx, y, k)
    # # print(a / (k + 1), b / (k + 1))
    # before[2] = before[2] + a/(k+1)
    # after[2] = after[2] + b/(k+1)

    state_all4 = []
    for j in range(length // 4):
        state_all4.append(state_all[j * 4] * 1000 + state_all[j * 4 + 1] * 100 + state_all[j * 4 + 2] * 10 + state_all[j * 4 + 3])
    x = []
    y = []
    for v in set(state_all4):
        c = state_all4.count(v)
        x.append(v)
        y.append(c)
    # print(x)
    xy = np.vstack((np.array(x), np.array(y)))
    xy = -xy.T
    xy = xy[xy[:, 1].argsort(-1)]
    xy = -xy
    x = list(xy[:, 0])
    y = xy[:, 1]
    y = y / sum(y)

    k = 3
    rep = [3333, 3330, 3331, 3332, 3334, 3335, 3336, 3337,
           3303, 3313, 3323, 3343, 3353, 3363, 3373,
           3033, 3133, 3233, 3433, 3533, 3633, 3733,
           333, 1333, 2333, 4333, 5333, 6333, 7333]
    rep = [3333, 3330, 3331, 3332, 3334, 3335, 3336, 3337,
           3303, 3313, 3323, 3343, 3353, 3363, 3373,
           3033, 3133, 3233, 3433, 3533, 3633, 3733,
           333, 1333, 2333, 4333, 5333, 6333, 7333,
           33, 1133, 2233, 4433, 5533, 6633, 7733, 1033, 1233, 1433, 1533, 1633, 1733, 2133]
    rep = [3344, 4433, 4343, 3434, 3443, 4334, 3333, 4444, 3334, 4443, 3343, 4434, 3433, 4344,
           3444, 4333, 3330, 4440, 3331, 4441, 3332, 4442, 3335, 4445, 3336, 4446, 3337, 4447,
           3313, 4414, 3303, 4404, 3323, 4424, 3353, 4454, 3363, 4464, 3373, 4474, 3033, 4044,
           3133, 4144, 3233, 4244, 3533, 4544, 3633, 4644, 3733, 4744]

    mapx = mapping(x, y, rep)
    a = count_three(x, y, k, 3)
    b = count_three(mapx, y, k, 3)
    # print(a / (k + 1), b / (k + 1))
    before[3] = before[3] + a/(k+1)
    after[3] = after[3] + b/(k+1)
    # print(a/(k+1), b/(k+1))
    perc_3[i, 0] = b/(k+1)

    a = count_three(x, y, k, 4)
    b = count_three(mapx, y, k, 4)
    # print(a/(k+1), b/(k+1))
    before4[3] = before4[3] + a/(k+1)
    after4[3] = after4[3] + b/(k+1)

    # perc_3[i, 0] = a/(k+1)
    perc_3[i, 1] = b/(k+1)
    # exit(0)

print(before/(i+1))
print(after/(i+1))
print(before4/(i+1))
print(after4/(i+1))
print(i+1)

exit(0)
import statsmodels.api as sm
ecdf = sm.distributions.ECDF(perc_3[:, 0])  
x = np.linspace(min(perc_3[:, 0]), max(perc_3[:, 0]))
y = ecdf(x)
axs[0, 0].step(x, y, label='Before',color='r')
x = np.linspace(min(perc_3[:, 1]), max(perc_3[:, 1]))
y = ecdf(x)
axs[0, 1].step(x, y, label='After',color='b')
plt.ylim((0,1))
plt.show()


with open('testliqiao.csv', mode='w') as employee_file:
    employee_writer = csv.writer(employee_file, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
    for i in range(Num):
        employee_writer.writerow(perc_3[i])
exit(0)
state_all = []
for i in range(100*1000*1000//(pagesize*3)):
    LSB = fromfile.read(pagesize)
    CSB = fromfile.read(pagesize)
    MSB = fromfile.read(pagesize)
    for j in range(pagesize):
        state_all.append(form_state(LSB[j], CSB[j], MSB[j]))

length = state_all.__len__()
print(length)
# print(state_all)
state_all = np.array(state_all)
state_all = np.reshape(state_all, -1)
state_all = state_all.tolist()
# dic = {k:state_all.count(k) for k in set(state_all)}
# print(dic)

dist =[]
for k in set(state_all):
    x = state_all.count(k)
    dist.append([k, x])
dist = np.array(dist)
with open(fromname+'_one_chara.csv', mode='w') as employee_file:
    employee_writer = csv.writer(employee_file, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
    for i in range(dist.shape[0]):
        employee_writer.writerow(dist[i])

state_all2 = []
for i in range(length//2):
    state_all2.append(state_all[i*2]*10+state_all[i*2+1])
# dic2 = {k:state_all2.count(k) for k in set(state_all2)}
# print(dic2)
dist =[]
for k in set(state_all2):
    x = state_all2.count(k)
    dist.append([k, x])
dist = np.array(dist)
with open(fromname+'_two_chara.csv', mode='w') as employee_file:
    employee_writer = csv.writer(employee_file, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
    for i in range(dist.shape[0]):
        employee_writer.writerow(dist[i])

state_all3 = []
for i in range(length//3):
    state_all3.append(state_all[i*3]*100+state_all[i*3+1]*10+state_all[i*3+2])
# dic3 = {k:state_all3.count(k) for k in set(state_all3)}
# print(dic3)
dist =[]
for k in set(state_all3):
    x = state_all3.count(k)
    dist.append([k, x])
dist = np.array(dist)
with open(fromname+'_three_chara.csv', mode='w') as employee_file:
    employee_writer = csv.writer(employee_file, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
    for i in range(dist.shape[0]):
        employee_writer.writerow(dist[i])

state_all4 = []
for i in range(length//4):
    state_all4.append(state_all[i*4]*1000+state_all[i*4+1]*100+state_all[i*4+2]*10+state_all[i*4+3])
# dic4 = {k:state_all4.count(k) for k in set(state_all4)}
# print(dic4)
dist =[]
for k in set(state_all4):
    x = state_all4.count(k)
    dist.append([k, x])
dist = np.array(dist)
with open(fromname+'_four_chara.csv', mode='w') as employee_file:
    employee_writer = csv.writer(employee_file, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
    for i in range(dist.shape[0]):
        employee_writer.writerow(dist[i])

# sns.distplot(state_all, hist=False, kde_kws={"color":"red","linestyle":"-"}, norm_hist=True, label="original")
# sns.distplot(state_all2, hist=False, kde_kws={"color":"blue","linestyle":"--"}, norm_hist=True, label="output")

exit(0)


state = [7, 6, 4, 0, 2, 3, 1, 5]

h_state = [0, 0, 0, 0]
toname = 'pattern1'

tofile = open(toname, 'wb')

for k in range(WLnum):
    page_content = np.zeros((3, pagesize))

    for i in range(pagesize):
        v = np.zeros(8)
        for j in range(8):
            flag = random.randint(0, 1)
            flag = 0
            if flag == 0:
                ind = random.randint(0, 7)
                v[j] = state[ind]
            else:
                ind = random.randint(0, 3)
                v[j] = h_state[ind]
        page_content[:, i] = form_byte(v)

    for i in range(3):
        for j in range(pagesize):
            x = int(page_content[i, j])
            data = x.to_bytes(1, 'big')
            tofile.write(data)

    # print(page_content)
    print(page_content.shape)

tofile.close()
exit(0)


fromname = 'source'
print(fromname)
fromfile = open(fromname, 'rb')

toname = 'out_CSB_half'
tofile = open(toname, 'wb')


for i in range(384):
    for j in range(i*3*pagesize, (i*3+1)*pagesize):
        data = fromfile.read(1)
        tofile.write(data)
    for j in range((i*3+1)*pagesize, (i*3+2)*pagesize):
        data = fromfile.read(1)
        if j%2 == 0:
            a = 0
            data = a.to_bytes(1, 'big')
        tofile.write(data)

    for j in range((i*3+2)*pagesize, (i*3+3)*pagesize):
        data = fromfile.read(1)
        tofile.write(data)

fromfile.close()
tofile.close()


exit(0)


fromname = 'source'
print(fromname)
fromfile = open(fromname, 'rb')
for i in range(10):
    data = fromfile.read(1)
    num = struct.unpack('B', data)
    print(i, data, num)

toname = 'out'
tofile = open(toname, 'wb')
for i in range(10):
    data = 255
    content = data.to_bytes(1, 'big')
    tofile.write(content)



fromfile.close()
tofile.close()







fromname = 'out'
print(fromname)
fromfile = open(fromname, 'rb')
for i in range(10):
    data = fromfile.read(1)
    num = struct.unpack('B', data)
    print(i, data, num)

