# naive_bayes_mixture.py
# ---------------
# Licensing Information:  You are free to use or extend this projects for
# educational purposes provided that (1) you do not distribute or publish
# solutions, (2) you retain this notice, and (3) you provide clear
# attribution to the University of Illinois at Urbana-Champaign
#
# Created by Justin Lizama (jlizama2@illinois.edu) on 09/28/2018
# Modified by Jaewook Yeom 02/02/2020

"""
This is the main entry point for Part 2 of this MP. You should only modify code
within this file for Part 2 -- the unrevised staff files will be used for all other
files and classes when code is run, so be careful to not modify anything else.
"""
from collections import Counter
import math


def naiveBayesMixture(train_set, train_labels, dev_set, bigram_lambda,unigram_smoothing_parameter, bigram_smoothing_parameter, pos_prior):
    """
    train_set - List of list of words corresponding with each email
    example: suppose I had two emails 'like this movie' and 'i fall asleep' in my training set
    Then train_set := [['like','this','movie'], ['i','fall','asleep']]

    train_labels - List of labels corresponding with train_set
    example: Suppose I had two emails, first one was ham and second one was spam.
    Then train_labels := [1, 0]

    dev_set - List of list of words corresponding with each email that we are testing on
              It follows the same format as train_set

    bigram_lambda - float between 0 and 1

    unigram_smoothing_parameter - Laplace smoothing parameter for unigram model (between 0 and 1)

    bigram_smoothing_parameter - Laplace smoothing parameter for bigram model (between 0 and 1)

    pos_prior - positive prior probability (between 0 and 1)
    """

    # TODO: Write your code here
    # return predicted labels of development set

    # counters for Training Phase
    ham = Counter()
    ham_bi = Counter()
    spam = Counter()
    spam_bi = Counter()

    for string, label in zip(train_set, train_labels):
        for i in range(len(string)):
            word = string[i]
            if i != len(string)-1:
                word_bi = string[i] + ' ' + string[i+1]
                if label == 1:
                    ham_bi.update({word_bi:1})
                else:
                    spam_bi.update({word_bi:1})
            if label == 1:
                ham.update({word:1})
            else:
                spam.update({word:1})

    ham_len = 0
    for w in ham:
        ham_len += ham[w]
    spam_len = 0
    for w in spam:
        spam_len += spam[w]
        
    hambi_len = 0
    for w in ham_bi:
        hambi_len += ham_bi[w]
    spambi_len = 0
    for w in spam_bi:
        spambi_len += spam_bi[w]

    # labels for Development Phase
    dev_labels = []
    # dicts for P(word|ham) and P(word|spam)
    p_ham = {}
    p_spam = {}
    p_hambi = {}
    p_spambi = {}

    # develop likelihoods based on dev_set
    for word in ham:
        numerator = ham[word] + unigram_smoothing_parameter
        denominator = ham_len + unigram_smoothing_parameter*(len(ham))
        p_ham[word] = numerator / denominator
    for word in spam:
        numerator = spam[word] + unigram_smoothing_parameter
        denominator = spam_len + unigram_smoothing_parameter*(len(spam))
        p_spam[word] = numerator / denominator

    for word_bi in ham_bi:
        numerator = ham_bi[word_bi] + bigram_smoothing_parameter
        denominator = hambi_len + bigram_smoothing_parameter*(len(ham_bi))
        p_hambi[word_bi] = numerator / denominator
    for word_bi in spam_bi:
        numerator = spam_bi[word_bi] + bigram_smoothing_parameter
        denominator = spambi_len + bigram_smoothing_parameter*(len(spam_bi))
        p_spambi[word_bi] = numerator / denominator
    
    numerator = unigram_smoothing_parameter
    denominator = ham_len + unigram_smoothing_parameter*(len(ham))
    p_ham_zero = numerator / denominator
    denominator = spam_len + unigram_smoothing_parameter*(len(spam))
    p_spam_zero = numerator / denominator

    numerator = bigram_smoothing_parameter
    denominator = hambi_len + bigram_smoothing_parameter*(len(ham_bi))
    p_hambi_zero = numerator / denominator
    denominator = spambi_len + bigram_smoothing_parameter*(len(spam_bi))
    p_spambi_zero = numerator / denominator

    for string in dev_set:
        p_words_ham = math.log(pos_prior)
        p_words_spam = math.log(1 - pos_prior)

        p_words_hambi = math.log(pos_prior)
        p_words_spambi = math.log(1 - pos_prior)
        
        for i in range(len(string)):
            word = string[i]
            if word in p_ham:
                p_words_ham += math.log(p_ham[word])
            else:
                p_words_ham += math.log(p_ham_zero)
            if word in p_spam:
                p_words_spam += math.log(p_spam[word])
            else:
                p_words_spam += math.log(p_spam_zero)

            if i != len(string)-1:
                word_bi = string[i] + ' ' + string[i+1]
                if word_bi in p_hambi:
                    p_words_hambi += math.log(p_hambi[word_bi])
                else:
                    p_words_hambi += math.log(p_hambi_zero)
                if word_bi in p_spambi:
                    p_words_spambi += math.log(p_spambi[word_bi])
                else:
                    p_words_spambi += math.log(p_spambi_zero)

            p_ham_mix = p_words_ham*(1-bigram_lambda) + p_words_hambi*bigram_lambda
            p_spam_mix = p_words_spam*(1-bigram_lambda) + p_words_spambi*bigram_lambda

        dev_labels.append(p_ham_mix >= p_spam_mix)

    return dev_labels
