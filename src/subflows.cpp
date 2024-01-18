#include "subflows.hpp"

template <Index id>
void SubflowPhi(cme_internal_node * const node, const blas_ops &blas, const double tau, const integration_method &method)
{
    Index id_c = (id == 0) ? 1 : 0;

    gram_schmidt gs(&blas);
    multi_array<double, 2> Qmat({node->RankIn() * node->RankOut()[id_c], node->RankOut()[id]});
    std::function<double(double *, double *)> ip;
    ip = inner_product_from_const_weight(1.0, node->RankIn() * node->RankOut()[id_c]);

    // Compute QR decomposition C^n = G^n * (S^(n+id))^T
    Matrix::Matricize(node->Q, Qmat, id);
    gs(Qmat, node->child[id]->S, ip);
    Matrix::Tensorize(Qmat, node->G, id);
    transpose_inplace(node->child[id]->S);

    node->CalculateAB<id>(blas);

    if (node->child[id]->IsExternal())
    {
        cme_external_node *child_node = (cme_external_node *)node->child[id];

        // Compute coefficients C and D
        child_node->CalculateCD();

        // Compute K = X * S
        multi_array<double, 2> tmp_x(child_node->X);
        blas.matmul(tmp_x, child_node->S, child_node->X);

        // K step
        const auto K_step_rhs = [child_node](const multi_array<double, 2> &K) { return CalculateKDot(K, child_node); };
        method.integrate(child_node->X, K_step_rhs, tau);

        // Perform the QR decomposition K = X * S
        std::function<double(double *, double *)> ip_x;
        ip_x = inner_product_from_const_weight(child_node->grid.h_mult, child_node->grid.dx);
        gs(child_node->X, child_node->S, ip_x);
    }
    else
    {
        cme_internal_node *child_node = (cme_internal_node *)node->child[id];

        // Set C^(n+i) = Q^(n+id) * S^(n+id)
        multi_array<double, 2> Cmat_child({prod(child_node->RankOut()), child_node->RankIn()});
        multi_array<double, 2> Qmat_child({prod(child_node->RankOut()), child_node->RankIn()});
        Matrix::Matricize(child_node->Q, Qmat_child, 2);
        set_zero(Cmat_child);
        blas.matmul(Qmat_child, child_node->S, Cmat_child);
        Matrix::Tensorize(Cmat_child, child_node->Q, 2);

        TTNIntegrator(child_node, blas, tau, method);

        // Compute QR decomposition C^(n+id) = Q^(n+id) * S^(n+id)
        std::function<double(double *, double *)> ip_child;
        ip_child = inner_product_from_const_weight(1.0, prod(child_node->RankOut()));
        Matrix::Matricize(child_node->Q, Cmat_child, 2);
        gs(Cmat_child, child_node->S, ip_child);
        Matrix::Tensorize(Cmat_child, child_node->Q, 2);
    }

    // Integrate S
    node->child[id]->CalculateEF(blas);
    node->child[id]->CalculateS(tau);

    // Set C^n = G^n * (S^(n+id))^T
    multi_array<double, 2> Gmat({node->RankIn() * node->RankOut()[id_c], node->RankOut()[id]});
    Matrix::Matricize(node->G, Gmat, id);
    set_zero(Qmat);
    blas.matmul_transb(Gmat, node->child[id]->S, Qmat);
    Matrix::Tensorize(Qmat, node->Q, id);
}

template void SubflowPhi<0>(cme_internal_node * const node, const blas_ops &blas, const double tau, const integration_method &method);

template void SubflowPhi<1>(cme_internal_node * const node, const blas_ops &blas, const double tau, const integration_method &method);

void SubflowPsi(cme_internal_node * const node, const blas_ops &blas, const double tau)
{
    node->CalculateGH(blas);
    node->CalculateQ(tau);
}

void TTNIntegrator(cme_internal_node *node, const blas_ops &blas, const double tau, const integration_method &method)
{
    SubflowPhi<0>(node, blas, tau, method);
    SubflowPhi<1>(node, blas, tau, method);
    SubflowPsi(node, blas, tau);
}