function R = eigenvalue_allocation(X, M)

% Learn a projecting matrix R before product quantization

% This is the EigenValue Allocation in our CVPR 2013 paper:
% "Optimized Product Quantization for Approximate Nearest Neighbor Search"
% by Tiezheng Ge, Kaiming He, Qifa Ke, and Jian Sun.

% Input:
%   X = training data n-by-d
%   m = num of subspaces

% Output:
%   R = a d-by-d orthogonal matrix

n = size(X, 1);    %������������
dim = size(X, 2);  %����ά��

%%% remove mean
sample_mean = mean(X, 1);   %X��ֵ�ĸ��еľ�ֵ��
X = bsxfun(@minus, X, sample_mean);
%����XΪn*d��sample_meanΪ1*d�ģ����Խ�sample_mean���⸴��n�У��ٽ��м�����
%��˵õ�X-sample_mean����X���б�׼������ȥ��ֵ

%%% pca projection
dim_pca = dim; %%% reduce dim if possible


% covX=zeros(dim,dim);
% sub_dim=dim/k;
% for i = 1: k
%     P=X(:,(i-1)*sub_dim+1:i*sub_dim);
%     for j=1 :k 
%         Q=X(:,(j-1)*sub_dim+1:j*sub_dim);
%     covX((i-1)*sub_dim+1:i*sub_dim,(j-1)*sub_dim+1:j*sub_dim)=P'*Q;
%     end;
% end;
% covX = covX / n;

covX= X'*X / n;
[eigVec, eigVal] = eigs(covX, dim_pca, 'LM');   %����dim_pca������ֵ��������ֵ
%eigVec����������eigVal��Ӧ������ֵ�ԽǾ���
eigVal = diag(eigVal); 

%%% re-order the eigenvalues
dim_ordered = balanced_partition(eigVal, M);

%%% re-order the eigenvectors
R = eigVec(:, dim_ordered);

return;