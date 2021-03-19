# naive_bayes.py
# ---------------
# Licensing Information:  You are free to use or extend this projects for
# educational purposes provided that (1) you do not distribute or publish
# solutions, (2) you retain this notice, and (3) you provide clear
# attribution to the University of Illinois at Urbana-Champaign
#
# Created by Justin Lizama (jlizama2@illinois.edu) on 09/28/2018

"""
This is the main entry point for Part 1 of this MP. You should only modify code
within this file for Part 1 -- the unrevised staff files will be used for all other
files and classes when code is run, so be careful to not modify anything else.
"""
from collections import Counter
import math


def naiveBayes(train_set, train_labels, dev_set, smoothing_parameter, pos_prior):
    """
    train_set - List of list of words corresponding with each email
    example: suppose I had two emails 'like this movie' and 'i fall asleep' in my training set
    Then train_set := [['like','this','movie'], ['i','fall','asleep']]

    train_labels - List of labels corresponding with train_set
    example: Suppose I had two emails, first one was ham and second one was spam.
    Then train_labels := [1, 0]

    dev_set - List of list of words corresponding with each email that we are testing on
              It follows the same format as train_set

    smoothing_parameter - The smoothing parameter --laplace (1.0 by default)
    pos_prior - positive prior probability (between 0 and 1)
    """
    # TODO: Write your code here
    # return predicted labels of development set

    # counters for Training Phase
    ham = Counter()
    spam = Counter()

    for string, label in zip(train_set, train_labels):
        for word in string:
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

    # labels for Development Phase
    dev_labels = []
    # dicts for P(word|ham) and P(word|spam)
    p_ham = {}
    p_spam = {}

    # develop likelihoods based on dev_set
    for word in ham:
        numerator = ham[word] + smoothing_parameter
        denominator = ham_len + smoothing_parameter*(len(ham))
        p_ham[word] = numerator / denominator

    for word in spam:
        numerator = spam[word] + smoothing_parameter
        denominator = spam_len + smoothing_parameter*(len(spam))
        p_spam[word] = numerator / denominator
    
    numerator = smoothing_parameter
    denominator = ham_len + smoothing_parameter*(len(ham))
    p_ham_zero = numerator / denominator
    denominator = spam_len + smoothing_parameter*(len(spam))
    p_spam_zero = numerator / denominator

    for string in dev_set:
        p_words_ham = math.log(pos_prior)
        p_words_spam = math.log(1 - pos_prior)
        for word in string:
            if word in p_ham:
                p_words_ham += math.log(p_ham[word])
            else:
                p_words_ham += math.log(p_ham_zero)

            if word in p_spam:
                p_words_spam += math.log(p_spam[word])
            else:
                p_words_spam += math.log(p_spam_zero)

        dev_labels.append(p_words_ham >= p_words_spam)

    return dev_labels
